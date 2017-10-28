#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "utils.h"

char* create_server_greeting(uint32_t thread_id)
{
    /* Server Greeting:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Protocol version (0x0A)
     * 0x05 0x07    MySQL version (5.7.19\0 in our case, variable length in general)
     * 0x0C 0x04    Thread ID
     * 0x10 0x09    Salt (8 random bytes + \0)
     * 0x19 0x02    Server capabilities (0xFF 0x7F)
     * 0x1B 0x01    Server language (0x21 = utf8 COLLATE utf8_general_ci)
     * 0x1C 0x02    Server status (0x02 0x00)
     * 0x1E 0x02    Extended server capabilities (0xFF 0x81)
     * 0x20 0x01    Authentication plugin length
     * 0x21 0x0A    Unused (zero)
     * 0x2B 0x0D    Salt (12 random bytes + \0)
     * 0x38 0x16    Authentication plugin ('mysql_native_password\0')
     */
    const char tpl[0x4E] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x4A, 0x00, 0x00, 0x00, 0x0A, '5',  '.',  '7',  '.',  '1',  '9',  0x00, 0xFF, 0xFF, 0xFF, 0xFF,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0xFF, 0xF7, 0x21, 0x02, 0x00, 0xFF, 0x81,
        0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 'm',  'y',  's',  'q',  'l',  '_',  'n',  'a',
        't',  'i',  'v',  'e',  '_',  'p',  'a',  's',  's',  'w',  'o',  'r',  'd',  0x00
    };

    char* result = calloc(1, sizeof(tpl));
    memcpy(result, tpl, sizeof(tpl));
    memcpy(result + 0x0C, &thread_id, sizeof(thread_id));

    for (int i=0; i<8; ++i) {
        result[0x10 + i] = rand();
    }

    for (int i=0; i<12; ++i) {
        result[0x2B + i] = rand();
    }

    return result;
}

char* create_ooo_error(unsigned char seq)
{
    /* Response Error 1156:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Error packet (0xFF)
     * 0x05 0x02    Error code (1156, 0x0484)
     * 0x07 0x06    SQL State (#08S01)
     * 0x0D         Error message
     */
    const char tpl[0x25] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x21, 0x00, 0x00, 0x03, 0xFF, 0x84, 0x04, '#',  '0',  '8',  'S',  '0',  '1',  'G',  'o',  't',
        ' ',  'p',  'a',  'c',  'k',  'e',  't',  's',  ' ',  'o',  'u',  't',  ' ',  'o',  'f',  ' ',
        'o',  'r',  'd',  'e',  'r'
    };

    char* result = calloc(1, sizeof(tpl));
    memcpy(result, tpl, sizeof(tpl));
    result[0x03] = seq;
    return result;
}

char* create_auth_switch_request(unsigned char seq)
{
    /* Auth Switch Request:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Auth switch request (0xFE)
     * 0x05 0x16    Auth method name (variable length, NUL-terminated)
     * 0x1B 0x15    Auth method data (20 bytes of random salt + \0)
     */
    const char tpl[0x30] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x2C, 0x00, 0x00, 0x02, 0xFE, 'm',  'y',  's',  'q',  'l',  '_',  'n',  'a',  't',  'i',  'v',
        'e',  '_',  'p',  'a',  's',  's',  'w',  'o',  'r',  'd',  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00
    };

    char* result = calloc(1, sizeof(tpl));
    memcpy(result, tpl, sizeof(tpl));
    result[0x03] = seq;

    for (int i=0; i<20; ++i) {
        result[0x1B + i] = rand();
    }

    return result;
}

char* create_auth_failed(unsigned char seq, const char* user, const char* server, int use_pwd)
{
    /*
0040               47 00 00 02 ff 15 04 23 32 38 30 30  ...9G......#2800
0050   30 41 63 63 65 73 73 20 64 65 6e 69 65 64 20 66  0Access denied f
0060   6f 72 20 75 73 65 72 20 27 72 6f 6f 74 27 40 27  or user 'root'@'
0070   6c 6f 63 61 6c 68 6f 73 74 27 20 28 75 73 69 6e  localhost' (usin
0080   67 20 70 61 73 73 77 6f 72 64 3a 20 4e 4f 29     g password: NO)
     */
    /* Response Error 1045:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Error packet (0xFF)
     * 0x05 0x02    Error code (1045, 0x0415)
     * 0x07 0x06    SQL State (#28000)
     * 0x0D         Error message
     */
    const char tpl[0x0D] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0xFF, 0x00, 0x00, 0x03, 0xFF, 0x15, 0x04, '#',  '2',  '8',  '0',  '0',  '0'
    };

    const char* error = "Access denied for user '%.48s'@'%s' (using password: %s)";
    char buf[4096];

    int n = snprintf(buf, 4096, error, user, server, use_pwd ? "YES" : "NO");
    assert(n < 4096);

    char* result  = calloc(1, sizeof(tpl) + n);
    uint16_t size = sizeof(tpl) + n - 4;
    memcpy(result, &size, sizeof(size));
    memcpy(result + sizeof(size), tpl + sizeof(size), sizeof(tpl) - sizeof(size));
    memcpy(result + sizeof(tpl),  buf, n);

    result[0x03] = seq;
    return result;
}

int make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags != -1) {
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
