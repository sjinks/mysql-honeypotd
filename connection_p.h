#ifndef CONNECTION_P_H
#define CONNECTION_P_H

#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ev.h>

enum conn_state_t {
    NEW_CONN,
    WRITING_GREETING,
    READING_AUTH,
    WRITING_ASR,
    WRITING_OOO,
    WRITING_AF,
    SLEEPING,
    DONE
};

struct connection_t {
    struct ev_loop* loop;
    ev_io io;
    ev_timer tmr;
    ev_timer delay;
    char* buffer;
    size_t size;
    size_t pos;
    uint32_t length;
    unsigned char sequence;
    enum conn_state_t state;
    char ip[INET6_ADDRSTRLEN];
    char my_ip[INET6_ADDRSTRLEN];
    char host[NI_MAXHOST];
    uint16_t port;
    uint16_t my_port;
};

#endif /* CONNECTION_P_H */
