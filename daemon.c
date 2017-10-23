#include <stddef.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "daemon.h"
#include "globals.h"

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

int daemonize(struct globals_t* g)
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
