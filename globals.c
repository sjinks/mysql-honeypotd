#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ev.h>
#include "globals.h"
#include "log.h"

struct globals_t globals;

void init_globals(struct globals_t* g)
{
    memset(g, 0, sizeof(struct globals_t));

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
            my_log(LOG_DAEMON | LOG_WARNING, "WARNING: Failed to delete the PID file %s: %s", g->pid_file, strerror(errno));
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

    if (g->sockets) {
        for (size_t i=0; i<g->nsockets; ++i) {
            if (g->sockets[i] >= 0) {
                shutdown(g->sockets[i], SHUT_RDWR);
                close(g->sockets[i]);
            }
        }

        free(g->sockets);
    }

    kill_pid_file(g);

    if (!g->no_syslog) {
        closelog();
    }

    if (g->bind_addresses) {
        for (size_t i=0; i<g->nsockets; ++i) {
            free(g->bind_addresses[i]);
        }

        free(g->bind_addresses);
    }

    free(g->bind_port);
    free(g->pid_file);
    free(g->daemon_name);
    free(g->chroot_dir);
    free(g->pid_base);
    free(g->server_ver);
}
