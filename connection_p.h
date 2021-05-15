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
    READING_ASR,
    WRITING_OOO,
    WRITING_AF,
    SLEEPING,
    DONE
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct connection_t {
    struct ev_loop* loop;
    ev_io io;
    ev_timer tmr;
    ev_timer delay;
    unsigned char* buffer;
    unsigned char* auth_failed;
    size_t size;
    size_t pos;
    uint32_t length;
    uint16_t port;
    uint16_t my_port;
    enum conn_state_t state;
    uint8_t sequence;
    char ip[INET6_ADDRSTRLEN];
    char my_ip[INET6_ADDRSTRLEN];
    char host[NI_MAXHOST];
};
#pragma clang diagnostic pop

#endif /* CONNECTION_P_H */
