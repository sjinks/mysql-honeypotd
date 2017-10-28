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

static void check_pid_file(struct globals_t* g)
{
    g->pid_fd = create_pid_file(g->pid_file);
    if (g->pid_fd == -1) {
        fprintf(stderr, "Error creating PID file %s\n", g->pid_file);
        exit(EXIT_FAILURE);
    }

    if (g->pid_fd == -2) {
        fprintf(stderr, "mysql-honeypotd is already running\n");
        exit(EXIT_SUCCESS);
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
    const int on = 1;
    struct sockaddr_in sin;
    int res;

    memset(&sin, 0, sizeof(sin));
    sin.sin_port = htons((uint16_t)atoi(g->bind_port));
    if (1 == inet_pton(AF_INET, g->bind_address, &sin.sin_addr)) {
        sin.sin_family = AF_INET;
    }
    else if (1 == inet_pton(AF_INET6, g->bind_address, &sin.sin_addr)) {
        sin.sin_family = AF_INET6;
    }
    else {
        fprintf(stderr, "ERROR: '%s' is not a valid address\n", g->bind_address);
        exit(EXIT_FAILURE);
    }

    g->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == g->socket) {
        fprintf(stderr, "ERROR: Failed to create socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (-1 == setsockopt(g->socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int))) {
        fprintf(stderr, "WARNING: setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
    }

    if (-1 == make_nonblocking(g->socket)) {
        fprintf(stderr, "ERROR: Failed to make the socket non-blocking: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    res = bind(g->socket, (struct sockaddr*)&sin, sizeof(sin));
    if (-1 == res) {
        fprintf(stderr, "ERROR: failed to bind(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    res = listen(g->socket, 1024);
    if (-1 == res) {
        fprintf(stderr, "ERROR: failed to listen(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static int main_loop(struct globals_t* g)
{
    ev_signal sigterm_watcher;
    ev_signal sigint_watcher;
    ev_signal sigquit_watcher;
    ev_io accept_watcher;

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
    openlog(globals.daemon_name, option, LOG_DAEMON);
    check_pid_file(&globals);
    create_socket(&globals);
    become_daemon(&globals);

    if (write_pid(globals.pid_fd)) {
        syslog(LOG_DAEMON | LOG_CRIT, "ERROR: Failed to write to the PID file: %m");
        return EXIT_FAILURE;
    }

    return main_loop(&globals);
}
