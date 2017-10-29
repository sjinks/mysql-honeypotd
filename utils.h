#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

int make_nonblocking(int fd);
int safe_accept(int fd, struct sockaddr* addr, socklen_t* addrlen);
ssize_t safe_read(int fd, void* buf, size_t count);
ssize_t safe_write(int fd, const void* buf, size_t count);
void get_ip_port(const struct sockaddr* addr, char* ipstr, uint16_t* port);

#endif /* UTILS_H */
