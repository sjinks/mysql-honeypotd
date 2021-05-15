#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

uint8_t* create_server_greeting(uint32_t thread_id, const char* server_ver);
uint8_t* create_ooo_error(uint8_t seq);
uint8_t* create_auth_switch_request(uint8_t seq);
uint8_t* create_auth_failed(uint8_t seq, const uint8_t* user, const char* server, int use_pwd);

#endif /* PROTOCOL_H */
