#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <ev.h>
#include "globals.h"
#include "cmdline.h"
#include "connection.h"
#include "daemon.h"
#include "pidfile.h"
#include "utils.h"

static void signal_callback(struct ev_loop* loop, ev_signal* w, int revents)
{
    syslog(LOG_DAEMON | LOG_INFO, "Got signal %d, shutting down", w->signum);
    ev_break(loop, EVBREAK_ALL);
}

static void cleanup(void)
{
    free_globals(&globals);
}

static void daemonize(struct globals_t* g)
{
    int res;

    res = drop_privs(g);
    if (res != 0) {
        switch (res) {
            case DP_NO_UNPRIV_ACCOUNT:
                fprintf(stderr, "ERROR: Failed to find an unprivileged account\n");
                break;

            case DP_GENERAL_FAILURE:
            default:
                fprintf(stderr, "ERROR: Failed to drop privileges\n");
                break;
        }

        exit(EXIT_FAILURE);
    }

    if (!g->foreground) {
        if (daemon(0, 0)) {
            perror("daemon");
            exit(EXIT_FAILURE);
        }
    }
}

static int main_loop(struct globals_t* g)
{
    const int on = 1;
    ev_signal sigterm_watcher;
    ev_signal sigint_watcher;
    ev_signal sigquit_watcher;
    ev_io accept_watcher;
    struct sockaddr_in sin;
    int res;

    sin.sin_port = htons((uint16_t)atoi(g->bind_port));
    if (1 == inet_pton(AF_INET,  g->bind_address, &sin.sin_addr)) {
        sin.sin_family = AF_INET;
    }
    else if (1 == inet_pton(AF_INET6,  g->bind_address, &sin.sin_addr)) {
        sin.sin_family = AF_INET6;
    }
    else {
        fprintf(stderr, "'%s' is not a valid address\n", g->bind_address);
        return EXIT_FAILURE;
    }

    g->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == g->socket) {
        perror("socket");
        return EXIT_FAILURE;
    }

    setsockopt(g->socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    make_nonblocking(g->socket);

    res = bind(g->socket, (struct sockaddr*)&sin, sizeof(sin));
    if (-1 == res) {
        perror("bind");
        return EXIT_FAILURE;
    }

    res = listen(g->socket, 1024);
    if (-1 == res) {
        perror("listen");
        return EXIT_FAILURE;
    }

    ev_signal_init(&sigterm_watcher, signal_callback, SIGTERM);
    ev_signal_init(&sigint_watcher,  signal_callback, SIGINT);
    ev_signal_init(&sigquit_watcher, signal_callback, SIGQUIT);
    ev_signal_start(g->loop, &sigterm_watcher);
    ev_signal_start(g->loop, &sigint_watcher);
    ev_signal_start(g->loop, &sigquit_watcher);

    ev_io_init(&accept_watcher, new_connection, g->socket, EV_READ);
    ev_io_start(g->loop, &accept_watcher);

    ev_run(g->loop, 0);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    int option = LOG_PID | LOG_CONS;
#ifdef LOG_PERROR
    option |= LOG_PERROR;
#endif

    init_globals(&globals);
    atexit(cleanup);
    parse_options(argc, argv, &globals);

    globals.pid_fd = create_pid_file(globals.pid_file);
    if (globals.pid_fd == -1) {
        fprintf(stderr, "Error creating PID file %s\n", globals.pid_file);
        return EXIT_FAILURE;
    }

    if (globals.pid_fd == -2) {
        fprintf(stderr, "mysql-honeypotd is already running\n");
        return EXIT_SUCCESS;
    }

    daemonize(&globals);
    openlog(globals.daemon_name, option, LOG_DAEMON);

    if (write_pid(globals.pid_fd)) {
        syslog(LOG_CRIT, "Failed to write to the PID file: %m");
        return EXIT_FAILURE;
    }

    return main_loop(&globals);
}
