#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

char* create_server_greeting(uint32_t thread_id);
char* create_ooo_error(unsigned char seq);
char* create_auth_switch_request(unsigned char seq);
char* create_auth_failed(unsigned char seq, const char* user, const char* server, int use_pwd);

int make_nonblocking(int fd);
int safe_accept(int fd, struct sockaddr* addr, socklen_t* addrlen);
ssize_t safe_read(int fd, void* buf, size_t count);
ssize_t safe_write(int fd, const void* buf, size_t count);
void get_ip_port(const struct sockaddr* addr, char* ipstr, uint16_t* port);

#endif /* UTILS_H */
