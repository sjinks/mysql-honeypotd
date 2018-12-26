#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "utils.h"

int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags != -1) {
        if (flags & O_NONBLOCK) {
            return 0;
        }

        flags |= O_NONBLOCK;
        return fcntl(fd, F_SETFL, flags);
    }

    return -1;
}

int safe_accept(int fd, struct sockaddr* addr, socklen_t* addrlen)
{
    int res;
    do {
        res = accept(fd, addr, addrlen);
    } while (-1 == res && (EINTR == errno));

    return res;
}

ssize_t safe_read(int fd, void* buf, size_t count)
{
    ssize_t n;
    do {
        n = read(fd, buf, count);
    } while (-1 == n && (EINTR == errno));

    if (-1 == n && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        n = -2;
    }

    return n;
}

ssize_t safe_write(int fd, const void* buf, size_t count)
{
    ssize_t n;
    do {
        n = write(fd, buf, count);
    } while (-1 == n && (EINTR == errno));

    if (-1 == n && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        n = -2;
    }

    return n;
}

void get_ip_port(const struct sockaddr* addr, char* ipstr, uint16_t* port)
{
    assert(addr  != NULL);
    assert(ipstr != NULL);
    assert(port  != NULL);

    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in* s = (const struct sockaddr_in*)addr;
        *port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, INET6_ADDRSTRLEN);
    }
    else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6* s = (const struct sockaddr_in6*)addr;
        *port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, INET6_ADDRSTRLEN);
    }
    else {
        /* Should not happen */
        *ipstr = '\0';
        *port  = 0;
        assert(0);
    }
}
