#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <ev.h>
#include "connection.h"
#include "connection_p.h"
#include "dfa.h"
#include "globals.h"
#include "log.h"
#include "utils.h"

static void kill_connection(struct connection_t* conn, struct ev_loop* loop)
{
    ev_io_stop(loop, &conn->io);
    ev_timer_stop(loop, &conn->tmr);
    ev_timer_stop(loop, &conn->delay);

    shutdown(conn->io.fd, SHUT_RDWR);
    close(conn->io.fd);

    if (conn->buffer) {
        free(conn->buffer);
    }

    my_log(LOG_DAEMON | LOG_WARNING, "Closing connection for %s:%u", conn->ip, (unsigned int)conn->port);
    free(conn);
}

static void connection_timeout(struct ev_loop* loop, ev_timer* w, int revents)
{
    struct connection_t* conn = (struct connection_t*)w->data;
    my_log(LOG_AUTH | LOG_WARNING, "Connection timed out for %s:%u", conn->ip, (unsigned int)conn->port);
    kill_connection(conn, loop);
}

static void connection_callback(struct ev_loop* loop, ev_io* w, int revents)
{
    struct connection_t* conn = (struct connection_t*)w->data;
    int ok;

    ev_io_stop(loop, w);
    ev_timer_again(loop, &conn->tmr);
    switch (conn->state) {
        case NEW_CONN:
            ok = handle_new_connection(conn, revents);
            break;

        case WRITING_GREETING:
        case WRITING_ASR:
            ok = handle_write(conn, revents, READING_AUTH);
            break;

        case READING_AUTH:
            ok = handle_auth(conn, revents);
            break;

        case SLEEPING:
            ok = 1;
            ev_timer_stop(loop, &conn->tmr);
            break;

        case WRITING_OOO:
        case WRITING_AF:
            ok = handle_write(conn, revents, DONE);
            break;

        default:
            /* Should not happen */
            ok = 0;
            assert(0);
            break;
    }

    if (!ok || (revents & EV_ERROR) || conn->state == DONE) {
        kill_connection(conn, loop);
    }
    else {
        ev_io_set(w, w->fd, ok);
        ev_io_start(loop, w);
    }
}

void new_connection(struct ev_loop* loop, struct ev_io* w, int revents)
{
    struct sockaddr sa;
    socklen_t len = sizeof(sa);
    if (revents & EV_READ) {
        int sock = accept(w->fd, &sa, &len);
        if (sock != -1) {
            if (-1 == make_nonblocking(sock)) {
                my_log(LOG_DAEMON | LOG_WARNING, "new_connection(): failed to make the accept()'ed socket non-blocking: %s", strerror(errno));
                close(sock);
                return;
            }

            struct connection_t* conn = (struct connection_t*)calloc(1, sizeof(struct connection_t));
            conn->loop  = loop;
            conn->state = NEW_CONN;
            ev_io_init(&conn->io, connection_callback, sock, EV_READ | EV_WRITE);
            ev_timer_init(&conn->tmr, connection_timeout, 0, 10);
            ev_timer_init(&conn->delay, do_auth_failed, globals.delay ? globals.delay : 0.01, 0);
            conn->io.data    = conn;
            conn->tmr.data   = conn;
            conn->delay.data = conn;

            get_ip_port(&sa, conn->ip, &conn->port);
            if (0 != getnameinfo(&sa, len, conn->host, sizeof(conn->host), NULL, 0, 0)) {
                assert(INET6_ADDRSTRLEN < NI_MAXHOST);
                memcpy(conn->host, conn->ip, INET6_ADDRSTRLEN);
            }

            len = sizeof(sa);
            if (-1 != getsockname(sock, &sa, &len)) {
                get_ip_port(&sa, conn->my_ip, &conn->my_port);
            }
            else {
                my_log(LOG_DAEMON | LOG_WARNING, "WARNING: getsockname() failed: %s", strerror(errno));
                conn->my_port = atoi(globals.bind_port);
                memcpy(conn->my_ip, "0.0.0.0", sizeof("0.0.0.0"));
            }

            my_log(
                LOG_DAEMON | LOG_INFO,
                "New connection from %s:%u [%s] to %s:%u",
                conn->ip, (unsigned int)conn->port, conn->host,
                conn->my_ip, (unsigned int)conn->my_port
            );

            ev_io_start(loop, &conn->io);
        }
        else {
            my_log(LOG_DAEMON | LOG_WARNING, "accept() failed: %s", strerror(errno));
        }
    }
}
