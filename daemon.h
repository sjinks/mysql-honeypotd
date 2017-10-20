#ifndef DAEMON_H
#define DAEMON_H

#define DP_NO_UNPRIV_ACCOUNT         1
#define DP_GENERAL_FAILURE           2

struct globals_t;

int drop_privs(struct globals_t* globals);

#endif /* DAEMON_H */
