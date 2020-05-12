#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "dfa.h"
#include "connection_p.h"
#include "globals.h"
#include "protocol.h"
#include "log.h"
#include "utils.h"
#include <unistd.h>

static int out_of_order(struct connection_t* conn, int mask)
{
    my_log(
        LOG_DAEMON | LOG_WARNING,
        "Packets are out of order or invalid from %s:%u connecting to %s:%u",
        conn->ip,
        (unsigned int)conn->port,
        conn->my_ip,
        (unsigned int)conn->my_port
    );

    free(conn->buffer);
    conn->buffer = create_ooo_error(conn->sequence + 1);
    conn->size   = (unsigned int)(conn->buffer[0]) + 4;
    conn->pos    = 0;
    conn->state  = WRITING_OOO;
    return handle_write(conn, mask, DONE);
}

int handle_new_connection(struct connection_t* conn, int mask)
{
    conn->buffer = create_server_greeting(++globals.thread_id, globals.server_ver);
    conn->size   = (unsigned int)(conn->buffer[0]) + 4;
    conn->pos    = 0;
    conn->state  = WRITING_GREETING;

    return handle_write(conn, mask, READING_AUTH);
}

int handle_write(struct connection_t* conn, int mask, int next)
{
    if (mask & EV_WRITE) {
        ssize_t n = safe_write(conn->io.fd, conn->buffer + conn->pos, conn->size - conn->pos);
        if (-2 == n) {
            return EV_WRITE;
        }

        if (n <= 0) {
            return 0;
        }

        conn->pos += (size_t)n;
        if (conn->pos == conn->size) {
            free(conn->buffer);
            conn->pos    = 0;
            conn->size   = 0;
            conn->buffer = NULL;
            conn->state  = (enum conn_state_t)next;
            return EV_READ;
        }
    }

    return EV_WRITE;
}

void do_auth_failed(struct ev_loop* loop, struct ev_timer* timer, int revents)
{
    struct connection_t* conn = (struct connection_t*)timer->data;

    conn->state = WRITING_AF;
    ev_feed_event(loop, &conn->io, EV_WRITE);
}

static int do_auth(struct connection_t* conn, int mask)
{
    /*
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x02    Client capabilities (caps & 0x08: schema present)
     * 0x06 0x02    Extended client capabilities (extcaps & 0x08: plugin authentication)
     * 0x08 0x04    Maximum packet size
     * 0x0C 0x01    Client character set
     * 0x0D 0x17    Unused, zero
     * 0x24 ??      User name (NUL-terminated)
     * XX   0x01    Password length
     * XX+1 ??      Password
     * YY   ??      Schema (NUL-terminated); only if caps & 0x08
     * ZZ   ??      Client authentication plugin (NUL-terminated)
     */
    /* NB: conn->buffer has the packet without the first four bytes */

    uint16_t caps;
    uint16_t extcaps;
    memcpy(&caps,    conn->buffer + 0x04 - 0x04, sizeof(uint16_t));
    memcpy(&extcaps, conn->buffer + 0x06 - 0x04, sizeof(uint16_t));

    char* user     = conn->buffer + 0x24 - 0x04;
    char* user_end = (char*)memchr(user, 0, conn->size - 0x24 + 0x04);
    if (user_end == NULL) {
        return out_of_order(conn, mask);
    }

    unsigned char pwd_len = (unsigned char)*(user_end + 1);
    char* pos             = user_end + 1 + 1 + pwd_len;

    if (caps & 0x08) {
        char* schema_end = (char*)memchr(pos, 0, conn->buffer + conn->size - pos);
        if (schema_end == NULL) {
            return out_of_order(conn, mask);
        }

        pos = schema_end + 1;
    }

    if (extcaps & 0x08) {
        char* plugin_end = (char*)memchr(pos, 0, conn->buffer + conn->size - pos);
        if (plugin_end == NULL) {
            return out_of_order(conn, mask);
        }

        if (plugin_end - pos != 21 || strcasecmp("mysql_native_password", pos)) {
            free(conn->buffer);
            conn->buffer = create_auth_switch_request(conn->sequence + 1);
            conn->size   = (unsigned int)(conn->buffer[0]) + 4;
            conn->pos    = 0;
            conn->state  = WRITING_ASR;
            return handle_write(conn, mask, READING_AUTH);
        }
    }

    my_log(
        LOG_AUTH | LOG_WARNING,
        "Access denied for user '%s' from %s:%u to %s:%u (using password: %s)",
        user,
        conn->ip,
        (unsigned int)conn->port,
        conn->my_ip,
        (unsigned int)conn->my_port,
        pwd_len > 0 ? "YES" : "NO"
    );

    char* tmp    = create_auth_failed(conn->sequence + 1, user, conn->host, pwd_len > 0);
    free(conn->buffer);
    conn->buffer = tmp;
    conn->state  = SLEEPING;
    conn->size   = (unsigned int)(conn->buffer[0]) + 4;
    conn->pos    = 0;
    ev_timer_start(conn->loop, &conn->delay);
    return EV_WRITE;
}

int handle_auth(struct connection_t* conn, int mask)
{
    if (mask & EV_READ) {
        ssize_t n;
        if (conn->buffer == NULL) {
            // Reading packet size
            n = safe_read(conn->io.fd, ((char*)&conn->length) + conn->pos, sizeof(conn->length) - conn->pos);
            if (-2 == n) {
                return EV_READ;
            }

            if (n <= 0) {
                return 0;
            }

            conn->pos += (size_t)n;
            if (conn->pos != sizeof(conn->length)) {
                return EV_READ;
            }

            conn->sequence = (unsigned char)(conn->length >> 24);
            conn->size     = conn->length & 0x00FFFFFF;
            conn->pos      = 0;

            if (conn->size > 4096) {
                return out_of_order(conn, mask);
            }

            conn->buffer = malloc(conn->size);
        }

        n = safe_read(conn->io.fd, conn->buffer + conn->pos, conn->size - conn->pos);
        if (-2 == n) {
            return EV_READ;
        }

        if (n <= 0) {
            return 0;
        }

        conn->pos += (size_t)n;
        if (conn->pos == conn->size) {
            return do_auth(conn, mask);
        }
    }

    return EV_READ;
}
