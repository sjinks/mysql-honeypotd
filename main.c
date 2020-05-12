#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include "globals.h"
#include "cmdline.h"
#include "connection.h"
#include "daemon.h"
#include "log.h"
#include "pidfile.h"
#include "utils.h"

static void signal_callback(struct ev_loop* loop, ev_signal* w, int revents)
{
    my_log(LOG_DAEMON | LOG_INFO, "Got signal %d, shutting down", w->signum);
    ev_break(loop, EVBREAK_ALL);
}

static void cleanup(void)
{
    free_globals(&globals);
}

static void check_pid_file(struct globals_t* g)
{
    if (g->pid_file) {
        g->pid_fd = create_pid_file(g->pid_file);
        if (g->pid_fd == -1) {
            fprintf(stderr, "Error creating PID file %s: %s\n", g->pid_file, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (g->pid_fd == -2) {
            fprintf(stderr, "mysql-honeypotd is already running\n");
            exit(EXIT_SUCCESS);
        }
    }
}

static void become_daemon(struct globals_t* g)
{
    int res = daemonize(g);
    if (res != DAEMONIZE_OK) {
        int err = errno;
        switch (res) {
            case DAEMONIZE_UNPRIV:
                fprintf(stderr, "ERROR: Failed to find an unprivileged account\n");
                break;

            case DAEMONIZE_CHROOT:
                fprintf(stderr, "ERROR: Failed to chroot(%s): %s\n", globals.chroot_dir, strerror(err));
                break;

            case DAEMONIZE_CHDIR:
                fprintf(stderr, "ERROR: Failed to chdir(%s): %s\n", globals.chroot_dir, strerror(err));
                break;

            case DAEMONIZE_DROP:
                fprintf(stderr, "ERROR: Failed to drop privileges\n");
                break;

            case DAEMONIZE_DAEMON:
                fprintf(stderr, "ERROR: Failed to daemonize: %s\n", strerror(err));
                break;
        }

        exit(EXIT_FAILURE);
    }
}

static void create_socket(struct globals_t* g)
{
    size_t good = 0;
    const int on = 1;
    struct sockaddr_in sin;
    int res;

    g->sockets = calloc(g->nsockets, sizeof(int));
    if (!g->sockets) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    uint16_t port = htons((uint16_t)atoi(g->bind_port));
    for (size_t i=0; i<g->nsockets; ++i) {
        memset(&sin, 0, sizeof(sin));
        sin.sin_port = port;
        if (1 == inet_pton(AF_INET, g->bind_addresses[i], &sin.sin_addr)) {
            sin.sin_family = AF_INET;
        }
        else if (1 == inet_pton(AF_INET6, g->bind_addresses[i], &sin.sin_addr)) {
            sin.sin_family = AF_INET6;
        }
        else {
            fprintf(stderr, "ERROR: '%s' is not a valid address\n", g->bind_addresses[i]);
            continue;
        }

        g->sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == g->sockets[i]) {
            fprintf(stderr, "ERROR: Failed to create socket: %s\n", strerror(errno));
            continue;
        }

        if (-1 == setsockopt(g->sockets[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))) {
            fprintf(stderr, "WARNING: setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
        }

        if (-1 == setsockopt(g->sockets[i], SOL_IP, IP_FREEBIND, &on, sizeof(int))) {
            fprintf(stderr, "WARNING: setsockopt(IP_FREEBIND) failed: %s\n", strerror(errno));
        }

        if (-1 == make_nonblocking(g->sockets[i])) {
            fprintf(stderr, "ERROR: Failed to make the socket non-blocking: %s\n", strerror(errno));
            close(g->sockets[i]);
            g->sockets[i] = -1;
            continue;
        }

        res = bind(g->sockets[i], (struct sockaddr*)&sin, sizeof(sin));
        if (-1 == res) {
            fprintf(stderr, "ERROR: failed to bind(): %s\n", strerror(errno));
            close(g->sockets[i]);
            g->sockets[i] = -1;
            continue;
        }

        res = listen(g->sockets[i], 1024);
        if (-1 == res) {
            fprintf(stderr, "ERROR: failed to listen(): %s\n", strerror(errno));
            close(g->sockets[i]);
            g->sockets[i] = -1;
            continue;
        }

        ++good;
    }

    if (!good) {
        fprintf(stderr, "ERROR: No listening sockets available\n");
        exit(EXIT_FAILURE);
    }
}

static int main_loop(struct globals_t* g)
{
    ev_signal sigterm_watcher;
    ev_signal sigint_watcher;
    ev_signal sigquit_watcher;
    ev_io* accept_watchers;

    ev_signal_init(&sigterm_watcher, signal_callback, SIGTERM);
    ev_signal_init(&sigint_watcher,  signal_callback, SIGINT);
    ev_signal_init(&sigquit_watcher, signal_callback, SIGQUIT);
    ev_signal_start(g->loop, &sigterm_watcher);
    ev_signal_start(g->loop, &sigint_watcher);
    ev_signal_start(g->loop, &sigquit_watcher);

    accept_watchers = calloc(g->nsockets, sizeof(ev_io));
    if (!accept_watchers) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    for (size_t i=0; i<g->nsockets; ++i) {
        ev_io_init(&accept_watchers[i], new_connection, g->sockets[i], EV_READ);
        if (g->sockets[i] != -1) {
            ev_io_start(g->loop, &accept_watchers[i]);
        }
    }

    ev_run(g->loop, 0);
    for (size_t i=0; i<g->nsockets; ++i) {
        if (accept_watchers[i].fd != -1) {
            ev_io_stop(g->loop, &accept_watchers[i]);
        }
    }

    free(accept_watchers);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    int option = LOG_PID | LOG_CONS;

    init_globals(&globals);
    atexit(cleanup);
    parse_options(argc, argv, &globals);

#ifdef LOG_PERROR
    option |= LOG_PERROR;
#endif

    if (!globals.no_syslog) {
        openlog(globals.daemon_name, option, LOG_DAEMON);
    }

    check_pid_file(&globals);
    create_socket(&globals);
    become_daemon(&globals);

    if (globals.pid_file && write_pid(globals.pid_fd)) {
        my_log(LOG_DAEMON | LOG_CRIT, "ERROR: Failed to write to the PID file: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    return main_loop(&globals);
}
