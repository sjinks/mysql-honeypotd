#ifndef PIDFILE_H
#define PIDFILE_H

int create_pid_file(const char* path);
int write_pid(int fd);

#endif /* PIDFILE_H */
