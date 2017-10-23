#ifndef DAEMON_H
#define DAEMON_H

struct globals_t;

#define DAEMONIZE_OK         0
#define DAEMONIZE_UNPRIV    -1
#define DAEMONIZE_CHROOT    -2
#define DAEMONIZE_CHDIR     -3
#define DAEMONIZE_DROP      -4
#define DAEMONIZE_DAEMON    -5

int daemonize(struct globals_t* g);

#endif /* DAEMON_H */
