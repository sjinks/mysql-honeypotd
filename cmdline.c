#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "globals.h"
#include "cmdline.h"

static struct option long_options[] = {
    { "address",    required_argument, NULL, 'b' },
    { "port",       required_argument, NULL, 'p' },
    { "pid",        required_argument, NULL, 'P' },
    { "name",       required_argument, NULL, 'n' },
    { "user",       required_argument, NULL, 'u' },
    { "group",      required_argument, NULL, 'g' },
    { "chroot",     required_argument, NULL, 'c' },
    { "delay",      required_argument, NULL, 'd' },
    { "foreground", no_argument,       NULL, 'f' },
    { "no-syslog",  no_argument,       NULL, 'x' },
    { "setver",     required_argument, NULL, 's' },
    { "help",       no_argument,       NULL, 'h' },
    { "version",    no_argument,       NULL, 'v' }
};

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
static void usage()
{
    printf(
        "Usage: mysql-honeypotd [options]...\n\n"
        "Low-interaction MySQL honeypot\n\n"
        "Mandatory arguments to long options are mandatory for short options too.\n"
        "  -b, --address ADDRESS the IP address to bind to (default: 0.0.0.0)\n"
        "                        (can be specified multiple times)\n"
        "  -p, --port PORT       the port to bind to (default: 3306)\n"
#ifndef MINIMALISTIC_BUILD
        "  -P, --pid FILE        the PID file\n"
        "  -n, --name NAME       the name of the daemon for syslog\n"
        "                        (default: mysql-honeypotd)\n"
        "  -u, --user USER       drop privileges and switch to this USER\n"
        "                        (default: daemon or nobody)\n"
        "  -g, --group GROUP     drop privileges and switch to this GROUP\n"
        "                        (default: daemon or nogroup)\n"
        "  -c, --chroot DIR      chroot() into the specified DIR\n"
        "  -f, --foreground      do not daemonize\n"
        "                        (forced if no PID file specified)\n"
        "  -x, --no-syslog       log messages only to stderr\n"
        "                        (only works with --foreground)\n"
#endif
        "  -s, --setver VERSION  report this MySQL server version\n"
        "                        (default: 8.0.19)\n"
        "  -d, --delay DELAY     Add DELAY seconds after each login attempt\n"
        "  -h, --help            display this help and exit\n"
        "  -v, --version         output version information and exit\n\n"
#ifndef MINIMALISTIC_BUILD
        "NOTES:\n"
        "  1. --user, --group, and --chroot options are honored only if\n"
        "     mysql-honeypotd is run as root\n"
        "  2. PID file can be outside of chroot\n"
        "  3. When using --name and/or --group, please make sure that\n"
        "     the PID file can be deleted by the target user\n"
        "\n"
#endif
        "Please report bugs here: <https://github.com/sjinks/mysql-honeypotd/issues>\n"
    );

    exit(EXIT_SUCCESS);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
static void version()
{
    printf(
        "mysql-honeypotd 1.0.0\n"
        "Copyright (c) 2017, 2021 Volodymyr Kolesnykov <volodymyr@wildwolf.name>\n"
        "License: MIT <http://opensource.org/licenses/MIT>\n"
    );

    exit(EXIT_SUCCESS);
}

static void check_alloc(void* p, const char* api)
{
    if (!p) {
        perror(api);
        exit(EXIT_FAILURE);
    }
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((malloc, returns_nonnull))
#endif
static char* my_strdup(const char *s)
{
    char* retval = strdup(s);
    check_alloc(retval, "strdup");
    return retval;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((malloc, returns_nonnull))
#endif
static char* my_strndup(const char *s, size_t n)
{
    char* retval = strndup(s, n);
    check_alloc(retval, "strndup");
    return retval;
}

static void set_defaults(struct globals_t* g)
{
    if (!g->server_ver) {
        g->server_ver = my_strdup("8.0.19");
    }

#ifndef MINIMALISTIC_BUILD
    if (!g->daemon_name) {
        g->daemon_name = my_strdup("mysql-honeypotd");
    }
#endif

    if (!g->nsockets) {
        g->nalloc   = 1;
        g->nsockets = 1;
        g->bind_addresses = calloc(g->nalloc, sizeof(char*));
        check_alloc(g->bind_addresses, "calloc");
        g->bind_addresses[0] = my_strdup("0.0.0.0");
    }

    if (!g->bind_port) {
        g->bind_port = my_strdup("3306");
    }

}

#ifndef MINIMALISTIC_BUILD
static void resolve_pid_file(struct globals_t* g)
{
    if (g->pid_file) {
        if (g->pid_file[0] != '/') {
            char buf[PATH_MAX+1];
            char* cwd    = getcwd(buf, PATH_MAX + 1);
            char* newbuf = NULL;

            /* If the current directory is not below the root directory of
             * the current process (e.g., because the process set a new
             * filesystem root using chroot(2) without changing its current
             * directory into the new root), then, since Linux 2.6.36,
             * the returned path will be prefixed with the string
             * "(unreachable)". Such behavior can also be caused by
             * an unprivileged user by changing the current directory into
             * another mount namespace. When dealing with paths from
             * untrusted sources, callers of these functions should consider
             * checking whether the returned path starts with '/' or '('
             * to avoid misinterpreting an unreachable path as a relative path.
             */
            if (cwd && cwd[0] == '/') {
                size_t cwd_len = strlen(cwd);
                size_t pid_len = strlen(g->pid_file);
                newbuf         = calloc(cwd_len + pid_len + 2, 1);
                check_alloc(newbuf, "calloc");
                memcpy(newbuf, cwd, cwd_len);
                newbuf[cwd_len] = '/';
                memcpy(newbuf + cwd_len + 1, g->pid_file, pid_len);
                free(g->pid_file);
                g->pid_file = newbuf;
            }
            else {
                fprintf(stderr, "ERROR: Failed to get the current directory: %s\n", strerror(errno));
                free(g->pid_file);
                exit(EXIT_FAILURE);
            }
        }

        {
            int e;
            char* pid_dir = my_strdup(g->pid_file);
            char* pid_fil = my_strdup(g->pid_file);
            char* dir     = dirname(pid_dir);
            char* file    = basename(pid_fil);
            g->pid_base   = my_strdup(file);
            g->piddir_fd  = open(dir, O_DIRECTORY);
            e             = errno;

            free(pid_fil);
            if (g->piddir_fd == -1) {
                fprintf(stderr, "ERROR: Failed to open directory %s: %s\n", dir, strerror(e));
                free(g->pid_file);
                free(pid_dir);
                g->pid_file = NULL;
                exit(EXIT_FAILURE);
            }

            free(pid_dir);
        }
    }
}
#endif

void parse_options(int argc, char** argv, struct globals_t* g)
{
    while (1) {
        int option_index = 0;
        int c = getopt_long(
            argc, 
            argv,
            (
                "b:p:d:s:hv"
#ifndef MINIMALISTIC_BUILD
                "P:n:u:g:c:fx"
#endif
            ),
            long_options,
            &option_index
        );

        if (-1 == c) {
            break;
        }

        switch (c) {
            case 'b':
                ++g->nsockets;
                if (g->nsockets > g->nalloc) {
                    char** tmp;
                    g->nalloc += 16;
                    tmp = realloc(g->bind_addresses, g->nalloc * sizeof(char*));
                    check_alloc(tmp, "realloc");
                    g->bind_addresses = tmp;
                }

                g->bind_addresses[g->nsockets-1] = my_strdup(optarg);
                break;

            case 'p':
                free(g->bind_port);
                g->bind_port = my_strdup(optarg);
                break;

            case 'd':
                g->delay = atoi(optarg);
                if (g->delay < 0) {
                    g->delay = 0;
                }

                break;

            case 's':
                free(g->server_ver);
                g->server_ver = my_strndup(optarg, 63);
                break;

            case 'h':
                usage();
                /* unreachable */
                /* no break */

            case 'v':
                version();
                /* unreachable */
                /* no break */

#ifndef MINIMALISTIC_BUILD
            case 'P':
                free(g->pid_file);
                g->pid_file = my_strdup(optarg);
                break;

            case 'n':
                free(g->daemon_name);
                g->daemon_name = my_strdup(optarg);
                break;

            case 'f':
                g->foreground = 1;
                break;

            case 'x':
                g->no_syslog = 1;
                break;

            case 'u': {
                struct passwd* pwd = getpwnam(optarg);
                if (!pwd) {
                    fprintf(stderr, "WARNING: unknown user %s\n", optarg);
                }
                else {
                    g->uid     = pwd->pw_uid;
                    g->uid_set = 1;
                    if (!g->gid_set) {
                        g->gid     = pwd->pw_gid;
                        g->gid_set = 1;
                    }
                }

                break;
            }

            case 'g': {
                struct group* grp = getgrnam(optarg);
                if (!grp) {
                    fprintf(stderr, "WARNING: unknown group %s\n", optarg);
                }
                else {
                    g->gid     = grp->gr_gid;
                    g->gid_set = 1;
                }

                break;
            }

            case 'c':
                free(g->chroot_dir);
                g->chroot_dir = my_strdup(optarg);
                break;
#endif

            case '?':
            default:
                break;
        }
    }

    while (optind < argc) {
        fprintf(stderr, "WARNING: unrecognized option: %s\n", argv[optind]);
        ++optind;
    }

    set_defaults(g);
#ifndef MINIMALISTIC_BUILD
    resolve_pid_file(g);

    if (!g->pid_file) {
        g->foreground = 1;
    }

    if (!g->foreground) {
        g->no_syslog = 0;
    }
#endif
}
