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
#include "log.h"    // IWYU pragma: keep
#include "pidfile.h"
#include "utils.h"  // IWYU pragma: keep

static void cleanup(void)
{
    free_globals(&globals);
}

static uint16_t parse_port(const char* port_str)
{
    char* endptr = NULL;
    long int port_long;

    errno = 0;
    port_long = strtol(port_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || port_long < 1 || port_long > 65535) {
        fprintf(stderr, "ERROR: Invalid port number: %s (must be 1-65535)\n", port_str);
        exit(EXIT_FAILURE);
    }

    return (uint16_t)port_long;
}

static void create_socket(struct globals_t* g)
{
    size_t good = 0;
    const int on = 1;
    int res;
    uint16_t port;

    union {
        struct sockaddr sa;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa_in6;
    } sin;

    g->sockets = calloc(g->nsockets, sizeof(int));
    if (!g->sockets) {
        fprintf(stderr, "ERROR: Failed to allocate memory for sockets\n");
        exit(EXIT_FAILURE);
    }

    port = parse_port(g->bind_port);
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

#ifdef IP_FREEBIND
        if (sin.sa.sa_family == AF_INET && -1 == setsockopt(g->sockets[i], IPPROTO_IP, IP_FREEBIND, &on, sizeof(int))) {
            fprintf(stderr, "WARNING: setsockopt(IP_FREEBIND) failed: %s\n", strerror(errno));
        }
#endif

#ifdef IPV6_FREEBIND
        if (sin.sa.sa_family == AF_INET6 && -1 == setsockopt(g->sockets[i], IPPROTO_IPV6, IPV6_FREEBIND, &on, sizeof(int))) {
            fprintf(stderr, "WARNING: setsockopt(IPV6_FREEBIND) failed: %s\n", strerror(errno));
        }
#endif

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
