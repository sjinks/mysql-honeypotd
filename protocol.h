#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

char* create_server_greeting(uint32_t thread_id, const char* server_ver);
char* create_ooo_error(unsigned char seq);
char* create_auth_switch_request(unsigned char seq);
char* create_auth_failed(unsigned char seq, const char* user, const char* server, int use_pwd);

#endif /* PROTOCOL_H */
