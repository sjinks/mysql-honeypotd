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

int drop_privs(struct globals_t* g)
{
    if (0 == geteuid()) {
        if (!g->uid_set || !g->gid_set) {
            uid_t uid;
            gid_t gid;

            if (-1 == find_account(&uid, &gid)) {
                return DP_NO_UNPRIV_ACCOUNT;
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

        if (
               setgroups(0, NULL)
            || setgid(g->gid)
            || setuid(g->uid)
        ) {
            return DP_GENERAL_FAILURE;
        }
    }

    return 0;
}
