#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <ev.h>
#include "globals.h"

struct globals_t globals;

void init_globals(struct globals_t* g)
{
    memset(g, 0, sizeof(struct globals_t));

    g->socket = -1;
    g->pid_fd = -1;
    g->loop   = EV_DEFAULT;

    signal(SIGPIPE, SIG_IGN);
}

void free_globals(struct globals_t* g)
{
    if (g->loop) {
        ev_loop_destroy(g->loop);
    }

    if (g->socket >= 0) {
        shutdown(g->socket, SHUT_RDWR);
        close(g->socket);
    }

    closelog();

    if (g->pid_fd >= 0) {
        assert(g->pid_file != NULL);
        unlink(g->pid_file);
        close(g->pid_fd);
    }

    if (g->bind_address) {
        free(g->bind_address);
    }

    if (g->bind_port) {
        free(g->bind_port);
    }

    if (g->pid_file) {
        free(g->pid_file);
    }

    if (g->daemon_name) {
        free(g->daemon_name);
    }
}
