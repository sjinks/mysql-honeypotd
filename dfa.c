#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "protocol.h"
#include "byteutils.h"
#include "connection_p.h"
#include "dfa.h"
#include "globals.h"
#include "log.h"
#include "utils.h"

/* capabilities */
#define CLIENT_CONNECT_WITH_DB                  0x0008
#define CLIENT_PROTOCOL_41                      0x0200

/* extended capabilities */
#define CLIENT_PLUGIN_AUTH                      0x0008
#define CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA   0x0020

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

/**
 * @see https://dev.mysql.com/doc/dev/mysql-server/8.0.11/page_protocol_basic_dt_integers.html#sect_protocol_basic_dt_int_le
 */
static uint64_t decodeLEI(const uint8_t* buffer, size_t buflen, size_t* bytes)
{
    assert(buflen > 0);
    assert(bytes != NULL);

    /* A fixed-length unsigned integer stores its value in a series of bytes with the least significant byte first. */
    switch (*buffer) {
        case 0x00u:
            *bytes = 1;
            return 0;

        case 0xFCu:
            *bytes = buflen > 2 ? 3 : 0;
            return *bytes ? load2(buffer + 1) : 0;

        case 0xFDu:
            *bytes = buflen > 3 ? 4 : 0;
            return *bytes ? load3(buffer + 1) : 0;

        case 0xFEu:
            *bytes = buflen > 8 ? 9 : 0;
            return *bytes ? load8(buffer + 1) : 0;

        case 0xFFu:
            *bytes = 0;
            return 0;

        default:
            *bytes = 1;
            return load1(buffer);
    }
}

static void log_access_denied(const struct connection_t* conn, const uint8_t* user, const uint8_t* auth_plugin, size_t pwd_len)
{
    my_log(
        LOG_AUTH | LOG_WARNING,
        "Access denied for user '%s' from %s:%u to %s:%u (using password: %s; authentication plugin: %s)",
        user,
        conn->ip,
        (unsigned int)conn->port,
        conn->my_ip,
        (unsigned int)conn->my_port,
        pwd_len > 0 ? "YES" : "NO",
        auth_plugin ? (const char*)auth_plugin : "N/A"
    );
}

/**
 * @see https://dev.mysql.com/doc/dev/mysql-server/8.0.11/page_protocol_connection_phase_packets_protocol_handshake_response.html
 */
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
     * XX   ??      Auth data
     * YY   ??      Schema (NUL-terminated); only if caps & 0x08
     * ZZ   ??      Client authentication plugin (NUL-terminated)
     */
    /* NB: conn->buffer has the packet without the first four bytes */

    uint16_t caps    = load2(conn->buffer + 0x04 - 0x04);
    uint16_t extcaps = load2(conn->buffer + 0x06 - 0x04);

    if ((caps & CLIENT_PROTOCOL_41) == 0) {
        /* We do not support the old HandshakeResponse320 yet */
        return out_of_order(conn, mask);
    }

    const uint8_t* user     = (conn->buffer + 0x24 - 0x04);
    const uint8_t* user_end = memchr(user, 0, conn->size - 0x24 + 0x04);
    if (user_end == NULL) {
        return out_of_order(conn, mask);
    }

    uint64_t pwd_len = 0;
    const uint8_t* pos = user_end + 1;
    /* const uint8_t* pwd_pos; */
    if (extcaps & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) {
        size_t bytes;
        pwd_len = decodeLEI(pos, (size_t)(conn->buffer + conn->size - pos), &bytes);
        if (!bytes) {
            return out_of_order(conn, mask);
        }

        /* pwd_pos = pos + bytes; */
        pos    += bytes + pwd_len;
    }
    else {
        pwd_len = load1(pos);
        /* pwd_pos = pos + 1; */
        pos    += 1 + pwd_len;
    }

    if (pos > conn->buffer + conn->size) {
        return out_of_order(conn, mask);
    }

    if (caps & CLIENT_CONNECT_WITH_DB) {
        uint8_t* schema_end = memchr(pos, 0, (size_t)(conn->buffer + conn->size - pos));
        if (schema_end == NULL) {
            return out_of_order(conn, mask);
        }

        pos = schema_end + 1;
    }

    const uint8_t* auth_plugin = NULL;
    if (extcaps & CLIENT_PLUGIN_AUTH) {
        const uint8_t* plugin_end = memchr(pos, 0, (size_t)(conn->buffer + conn->size - pos));
        if (plugin_end == NULL) {
            return out_of_order(conn, mask);
        }

        auth_plugin = pos;
        if (strcasecmp("mysql_clear_password", (const char*)pos) && strcasecmp("mysql_native_password", (const char*)pos)) {
            log_access_denied(conn, user, auth_plugin, pwd_len);

            free(conn->buffer);
            conn->buffer      = create_auth_switch_request(conn->sequence + 1);
            conn->size        = (unsigned int)(conn->buffer[0]) + 4;
            conn->pos         = 0;
            conn->state       = WRITING_ASR;
            conn->auth_failed = create_auth_failed(conn->sequence + 1, user, conn->host, pwd_len > 0);
            return handle_write(conn, mask, WRITING_ASR);
        }
    }

    log_access_denied(conn, user, auth_plugin, pwd_len);

    uint8_t* tmp = create_auth_failed(conn->sequence + 1, user, conn->host, pwd_len > 0);
    free(conn->buffer);
    conn->buffer = tmp;
    conn->state  = SLEEPING;
    conn->size   = (unsigned int)(conn->buffer[0]) + 4;
    conn->pos    = 0;
    ev_timer_start(conn->loop, &conn->delay);
    return EV_WRITE;
}

static int do_auth_asr(struct connection_t* conn, int mask)
{
    assert(conn->auth_failed != NULL);
    free(conn->buffer);
    conn->buffer      = conn->auth_failed;
    conn->auth_failed = NULL;
    conn->buffer[3]   = conn->sequence + 1;
    conn->state       = SLEEPING;
    conn->size        = (unsigned int)(conn->buffer[0]) + 4;
    conn->pos         = 0;
    ev_timer_start(conn->loop, &conn->delay);
    return EV_WRITE;
}

static int handle_read(struct connection_t* conn, int mask, int(*callback)(struct connection_t*, int))
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
            return callback(conn, mask);
        }
    }

    return EV_READ;
}

int handle_auth(struct connection_t* conn, int mask)
{
    return handle_read(conn, mask, do_auth);
}

int handle_auth_asr(struct connection_t* conn, int mask)
{
    return handle_read(conn, mask, do_auth_asr);
}
