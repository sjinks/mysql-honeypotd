#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

int make_nonblocking(int fd);
ssize_t safe_read(int fd, void* buf, size_t count);
ssize_t safe_write(int fd, const void* buf, size_t count);
void get_ip_port(const struct sockaddr_storage* addr, char* ipstr, uint16_t* port);
void fill_random(uint8_t* buffer, size_t length);

#endif /* UTILS_H */
