#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include "globals.h"
#include "cmdline.h"
#include "daemon.h"
#include "eventloop.h"
#include "log.h"
#include "pidfile.h"
#include "utils.h"

static void cleanup(void)
{
    free_globals(&globals);
}

static void create_socket(struct globals_t* g)
{
    size_t good = 0;
    const int on = 1;
    int res;

    union {
        struct sockaddr sa;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa_in6;
    } sin;

    g->sockets = calloc(g->nsockets, sizeof(int));
    if (!g->sockets) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    uint16_t port = htons((uint16_t)atoi(g->bind_port));
    for (size_t i=0; i<g->nsockets; ++i) {
        memset(&sin, 0, sizeof(sin));
        if (inet_pton(AF_INET, g->bind_addresses[i], &sin.sa_in.sin_addr) == 1) {
            sin.sa_in.sin_family = AF_INET;
            sin.sa_in.sin_port = htons(port);
        }
        else if (inet_pton(AF_INET6, g->bind_addresses[i], &sin.sa_in6.sin6_addr) == 1) {
            sin.sa_in6.sin6_family = AF_INET6;
            sin.sa_in6.sin6_port = htons(port);
        }
        else {
            g->sockets[i] = -1;
            fprintf(stderr, "ERROR: '%s' is not a valid address\n", g->bind_addresses[i]);
            free(g->bind_addresses[i]);
            g->bind_addresses[i] = NULL;
            continue;
        }

#if defined(__linux__) && defined(SOCK_NONBLOCK)
        g->sockets[i] = socket(sin.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
#else
        g->sockets[i] = socket(sin.sa.sa_family, SOCK_STREAM, 0);
#endif
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

#if !defined(__linux__) || !defined(SOCK_NONBLOCK)
        if (-1 == make_nonblocking(g->sockets[i])) {
            fprintf(stderr, "ERROR: Failed to make the socket non-blocking: %s\n", strerror(errno));
            close(g->sockets[i]);
            g->sockets[i] = -1;
            continue;
        }
#endif

        res = bind(g->sockets[i], &sin.sa, sizeof(sin));
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

int main(int argc, char** argv)
{
    init_globals(&globals);
    atexit(cleanup);
    parse_options(argc, argv, &globals);

#ifndef MINIMALISTIC_BUILD
    if (!globals.no_syslog) {
        openlog(
            globals.daemon_name,
            (
                  LOG_PID 
                | LOG_CONS
#ifdef LOG_PERROR
                | LOG_PERROR
#endif
            ),
            LOG_DAEMON
        );
    }

    check_pid_file(&globals);
#endif
    create_socket(&globals);
    become_daemon(&globals);

    return main_loop(&globals);
}
