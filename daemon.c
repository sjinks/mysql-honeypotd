#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <netdb.h>
#include "daemon.h"
#include "globals.h"
#include "log.h"
#include "pidfile.h"

#ifndef MINIMALISTIC_BUILD

#define DAEMONIZE_OK         0
#define DAEMONIZE_UNPRIV    -1
#define DAEMONIZE_CHROOT    -2
#define DAEMONIZE_CHDIR     -3
#define DAEMONIZE_DROP      -4
#define DAEMONIZE_DAEMON    -5

static void load_resolver()
{
    struct addrinfo *result = NULL;
    int res = getaddrinfo("localhost", NULL, NULL, &result);
    if (0 != res) {
        // Don't bail here; we will fall back to IP address
        my_log(LOG_DAEMON | LOG_CRIT, "Failed to load resolver data: %s", gai_strerror(res));
    }

    if (result) {
        freeaddrinfo(result);
    }
}

static int find_account(uid_t* uid, gid_t* gid)
{
    struct passwd* pwd;

    pwd = getpwnam("nobody");
    if (!pwd) {
        pwd = getpwnam("daemon");
    }

    if (pwd) {
        *uid = pwd->pw_uid;
        *gid = pwd->pw_gid;
        return 0;
    }

    return -1;
}

static int drop_privs(struct globals_t* g)
{
    if (
           setgroups(0, NULL)
        || setgid(g->gid)
        || setuid(g->uid)
    ) {
        return -1;
    }

    return 0;
}

static int daemonize(struct globals_t* g)
{
    if (geteuid() == 0) {
        if (!g->uid_set || !g->gid_set) {
            uid_t uid;
            gid_t gid;

            if (-1 == find_account(&uid, &gid)) {
                return DAEMONIZE_UNPRIV;
            }

            if (!g->uid_set) {
                g->uid_set = 1;
                g->uid     = uid;
            }

            if (!g->gid_set) {
                g->gid_set = 1;
                g->gid     = gid;
            }
        }

        if (g->chroot_dir) {
            load_resolver();
            if (-1 == chroot(g->chroot_dir)) {
                return DAEMONIZE_CHROOT;
            }

            if (-1 == chdir("/")) {
                return DAEMONIZE_CHDIR;
            }
        }

        if (-1 == drop_privs(g)) {
            return DAEMONIZE_DROP;
        }
    }

    if (!g->foreground && -1 == daemon(0, 0)) {
        return DAEMONIZE_DAEMON;
    }

    return DAEMONIZE_OK;
}
#endif

void become_daemon(struct globals_t* g)
{
#ifndef MINIMALISTIC_BUILD
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

    if (globals.pid_file && write_pid(globals.pid_fd)) {
        my_log(LOG_DAEMON | LOG_CRIT, "ERROR: Failed to write to the PID file: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
#endif
}
