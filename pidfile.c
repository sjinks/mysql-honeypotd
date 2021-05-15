#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "globals.h"
#include "pidfile.h"

#define BUF_SIZE 64

static int lock_file(int fd, short int type, short int whence, off_t start, off_t len)
{
    struct flock fl;

    fl.l_type   = type;
    fl.l_whence = whence;
    fl.l_start  = start;
    fl.l_len    = len;
    fl.l_pid    = 0;

#ifdef F_OFD_SETLK
    return fcntl(fd, F_OFD_SETLK, &fl);
#else
    return fcntl(fd, F_SETLK, &fl);
#endif
}

void check_pid_file(struct globals_t* g)
{
#ifndef MINIMALISTIC_BUILD
    if (g->pid_file) {
        g->pid_fd = create_pid_file(g->pid_file);
        if (g->pid_fd == -1) {
            fprintf(stderr, "Error creating PID file %s: %s\n", g->pid_file, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (g->pid_fd == -2) {
            fprintf(stderr, "mysql-honeypotd is already running\n");
            exit(EXIT_SUCCESS);
        }
    }
#endif
}

int create_pid_file(const char* path)
{
    int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        return -1;
    }

    if (lock_file(fd, F_WRLCK, SEEK_SET, 0, 0) == -1) {
        int e = errno;
        close(fd);
        errno = e;
        if (e == EAGAIN || e == EACCES) {
            /* PID file locked - another instance is running */
            return -2;
        }

        return -1;
    }

    if (ftruncate(fd, 0) == -1) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }

#ifndef F_OFD_SETLK
    if (lock_file(fd, F_UNLCK, SEEK_SET, 0, 0) == -1) {
        assert(0);
    }
#endif

    return fd;
}

int write_pid(int fd)
{
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "%ld\n", (long int)getpid());

#ifndef F_OFD_SETLK
    if (lock_file(fd, F_WRLCK, SEEK_SET, 0, 0)) {
        return -1;
    }
#endif

    if (write(fd, buf, strlen(buf)) != (ssize_t)strlen(buf)) {
        return -1;
    }

    return fsync(fd);
}
