#ifndef PIDFILE_H
#define PIDFILE_H

struct globals_t;
void check_pid_file(struct globals_t* g);
int create_pid_file(const char* path);
int write_pid(int fd);

#endif /* PIDFILE_H */
