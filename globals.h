#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <sys/types.h>

struct globals_t {
    struct ev_loop* loop;
    char* bind_address;
    char* bind_port;
    char* pid_file;
    char* daemon_name;
    int socket;
    int pid_fd;
    int foreground;
    int uid_set;
    int gid_set;
    uid_t uid;
    gid_t gid;
    uint32_t thread_id;
};

extern struct globals_t globals;

void init_globals(struct globals_t* g);
void free_globals(struct globals_t* g);

#endif /* GLOBALS_H */
