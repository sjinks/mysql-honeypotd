#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <ev.h>
#include "globals.h"

struct globals_t globals;

void init_globals(struct globals_t* g)
{
    memset(g, 0, sizeof(struct globals_t));

    g->socket    = -1;
    g->pid_fd    = -1;
    g->piddir_fd = -1;
    g->loop      = EV_DEFAULT;

    signal(SIGPIPE, SIG_IGN);
}

static void kill_pid_file(struct globals_t* g)
{
    if (g->pid_fd >= 0) {
        assert(g->pid_file != NULL);
        assert(g->pid_base != NULL);

        if (-1 == unlinkat(g->piddir_fd, g->pid_base, 0)) {
            syslog(LOG_DAEMON | LOG_WARNING, "WARNING: Failed to delete the PID file %s: %m", g->pid_file);
        }

        close(g->pid_fd);
        close(g->piddir_fd);
    }
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

    kill_pid_file(g);

    closelog();

    free(g->bind_address);
    free(g->bind_port);
    free(g->pid_file);
    free(g->daemon_name);
    free(g->chroot_dir);
    free(g->pid_base);
}
