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
    { "address",   required_argument, NULL, 'b' },
    { "port",      required_argument, NULL, 'p' },
    { "pid",       required_argument, NULL, 'P' },
    { "name",      required_argument, NULL, 'n' },
    { "user",      required_argument, NULL, 'u' },
    { "chroot",    required_argument, NULL, 'c' },
    { "group",     required_argument, NULL, 'g' },
    { "help",      no_argument,       NULL, 'h' },
    { "version",   no_argument,       NULL, 'v' }
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
        "  -p, --port PORT       the port to bind to (default: 3306)\n"
        "  -P, --pid FILE        the PID file\n"
        "                        (default: /run/mysql-honeypotd/mysql-honeypotd.pid)\n"
        "  -n, --name NAME       the name of the daemon for syslog\n"
        "                        (default: mysql-honeypotd)\n"
        "  -u, --user USER       drop privileges and switch to this USER\n"
        "                        (default: daemon or nobody)\n"
        "  -g, --group GROUP     drop privileges and switch to this GROUP\n"
        "                        (default: daemon or nogroup)\n"
        "  -c, --chroot DIR      chroot() into the specified DIR\n"
        "  -f, --foreground      do not daemonize\n"
        "  -h, --help            display this help and exit\n"
        "  -v, --version         output version information and exit\n\n"
        "NOTES:\n"
        "  1. --user, --group, and --chroot options are honored only if\n"
        "     mysql-honeypotd is run as root\n"
        "  2. PID file can be outside of chroot\n"
        "  3. When using --name and/or --group, please make sure that\n"
        "     the PID file can be deleted by the target user\n"
        "\n"
        "Please report bugs here: <https://github.com/sjinks/mysql-honeypotd/issues>\n"
    );

    exit(0);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
static void version()
{
    printf(
        "mysql-honeypotd 0.1\n"
        "Copyright (c) 2017, Volodymyr Kolesnykov <volodymyr@wildwolf.name>\n"
        "License: MIT <http://opensource.org/licenses/MIT>\n"
    );

    exit(0);
}

static void set_defaults(struct globals_t* g)
{
    if (!g->bind_address) {
        g->bind_address = strdup("0.0.0.0");
    }

    if (!g->bind_port) {
        g->bind_port = strdup("3306");
    }

    if (!g->daemon_name) {
        g->daemon_name = strdup("mysql-honeypotd");
    }

    if (!g->pid_file) {
        g->pid_file = strdup("/run/mysql-honeypotd/mysql-honeypotd.pid");
    }
}

static void resolve_pid_file(struct globals_t* g)
{
    if (g->pid_file) {
        if (g->pid_file[0] != '/') {
            char buf[PATH_MAX+1];
            char* cwd    = getcwd(buf, PATH_MAX + 1);
            char* newbuf = NULL;

            if (cwd) {
                size_t cwd_len = strlen(cwd);
                size_t pid_len = strlen(g->pid_file);
                newbuf         = calloc(cwd_len + pid_len + 2, 1);
                if (newbuf) {
                    memcpy(newbuf, cwd, cwd_len);
                    newbuf[cwd_len] = '/';
                    memcpy(newbuf + cwd_len + 1, g->pid_file, pid_len);
                }
                else {
                    fprintf(stderr, "ERROR: calloc(): out of memory\n");
                    free(g->pid_file);
                    g->pid_file = NULL;
                    return;
                }
            }

            free(g->pid_file);
            g->pid_file = newbuf;
        }

        {
            int e;
            char* pid_dir = strdup(g->pid_file);
            char* pid_fil = strdup(g->pid_file);
            char* dir     = dirname(pid_dir);
            char* file    = basename(pid_fil);
            g->pid_base   = strdup(file);
            g->piddir_fd  = open(dir, O_DIRECTORY);
            e             = errno;

            free(pid_fil);
            if (g->piddir_fd == -1) {
                fprintf(stderr, "ERROR: Failed to open directory %s: %s\n", dir, strerror(e));
                free(g->pid_file);
                free(pid_dir);
                g->pid_file = NULL;
                return;
            }

            free(pid_dir);
        }
    }
}

void parse_options(int argc, char** argv, struct globals_t* g)
{
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "b:p:P:n:u:g:c:fhv", long_options, &option_index);
        if (-1 == c) {
            break;
        }

        switch (c) {
            case 'b':
                free(g->bind_address);
                g->bind_address = strdup(optarg);
                break;

            case 'p':
                free(g->bind_port);
                g->bind_port = strdup(optarg);
                break;

            case 'P':
                free(g->pid_file);
                g->pid_file = strdup(optarg);
                break;

            case 'n':
                free(g->daemon_name);
                g->daemon_name = strdup(optarg);
                break;

            case 'f':
                g->foreground = 1;
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
                g->chroot_dir = strdup(optarg);
                break;

            case 'h':
                usage();
                /* unreachable */
                /* no break */

            case 'v':
                version();
                /* unreachable */
                /* no break */

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
    resolve_pid_file(g);

    if (!g->bind_address || !g->bind_port || !g->daemon_name || !g->pid_file) {
        exit(1);
    }
}
