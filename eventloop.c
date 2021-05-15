#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <ev.h>
#include <syslog.h>
#include "eventloop.h"
#include "connection.h"
#include "globals.h"
#include "log.h"

static void signal_callback(struct ev_loop* loop, ev_signal* w, int revents)
{
    my_log(LOG_DAEMON | LOG_INFO, "Got signal %d, shutting down", w->signum);
    ev_break(loop, EVBREAK_ALL);
}

int main_loop(struct globals_t* g)
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
