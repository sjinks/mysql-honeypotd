#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include "byteutils.h"

uint8_t* create_server_greeting(uint32_t thread_id, const char* server_ver)
{
    /* Server Greeting:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Protocol version (0x0A)
     * 0x05 0x07    MySQL version (8.0.19\0 in our case, variable length in general)
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
    const uint8_t part1[3] = {
        0x00, 0x00, 0x0A
    };

    const uint8_t part2[0x36] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x00, 0xFF, 0xF7, 0x21, 0x02, 0x00, 0xFF, 0x81, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        'm',  'y',  's',  'q',  'l',  '_',  'n',  'a',  't',  'i',  'v',  'e',  '_',  'p',  'a',  's',
        's',  'w',  'o',  'r',  'd',  0x00
    };

    size_t ver_len   = strlen(server_ver) + 1;
    size_t offset    = 2 /* payload_size */ + sizeof(part1) + ver_len;
    size_t pkt_size  = 2 /* payload_size */ + sizeof(part1) + ver_len + 4 /* thread_id */ + 8 /* salt */ + sizeof(part2);
    uint16_t pl_size = (uint16_t)(pkt_size - 4);
    uint8_t* result  = calloc(1, pkt_size);

    store2(result, pl_size);
    memcpy(result + sizeof(pl_size),                 part1,      sizeof(part1));
    memcpy(result + sizeof(pl_size) + sizeof(part1), server_ver, ver_len);
    store4(result + offset, thread_id);
    memcpy(result + offset + 4 + 8,                  part2,      sizeof(part2));

    for (size_t i=0; i<8; ++i) {
        result[offset + 4 + i] = (uint8_t)rand();
    }

    for (size_t i=0; i<12; ++i) {
        result[offset + 12 + 0x13 + i] = (uint8_t)rand();
    }

    return result;
}

uint8_t* create_ooo_error(uint8_t seq)
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
    const uint8_t tpl[0x25] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x21, 0x00, 0x00, 0x03, 0xFF, 0x84, 0x04, '#',  '0',  '8',  'S',  '0',  '1',  'G',  'o',  't',
        ' ',  'p',  'a',  'c',  'k',  'e',  't',  's',  ' ',  'o',  'u',  't',  ' ',  'o',  'f',  ' ',
        'o',  'r',  'd',  'e',  'r'
    };

    uint8_t* result = calloc(1, sizeof(tpl));
    memcpy(result, tpl, sizeof(tpl));
    result[0x03] = seq;
    return result;
}

uint8_t* create_auth_switch_request(uint8_t seq)
{
    /* Auth Switch Request:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Auth switch request (0xFE)
     * 0x05 0x16    Auth method name (variable length, NUL-terminated)
     * 0x1B 0x15    Auth method data (20 bytes of random salt + \0)
     */
    const uint8_t tpl[0x30] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0x2C, 0x00, 0x00, 0x02, 0xFE, 'm',  'y',  's',  'q',  'l',  '_',  'n',  'a',  't',  'i',  'v',
        'e',  '_',  'p',  'a',  's',  's',  'w',  'o',  'r',  'd',  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00
    };

    uint8_t* result = calloc(1, sizeof(tpl));
    memcpy(result, tpl, sizeof(tpl));
    result[0x03] = seq;

    for (size_t i=0; i<20; ++i) {
        result[0x1B + i] = (uint8_t)rand();
    }

    return result;
}

uint8_t* create_auth_failed(uint8_t seq, const uint8_t* user, const char* server, int use_pwd)
{
    /* Response Error 1045:
     *
     * 0x00 0x03    Packet length
     * 0x03 0x01    Sequence #
     * 0x04 0x01    Error packet (0xFF)
     * 0x05 0x02    Error code (1045, 0x0415)
     * 0x07 0x06    SQL State (#28000)
     * 0x0D         Error message
     */
    const uint8_t tpl[0x0D] = {
    /*  00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F   */
        0xFF, 0x00, 0x00, 0x03, 0xFF, 0x15, 0x04, '#',  '2',  '8',  '0',  '0',  '0'
    };

    char buf[4096];
    uint8_t* result;
    uint16_t size;

    int n = snprintf(buf, 4096, "Access denied for user '%.48s'@'%s' (using password: %s)", user, server, use_pwd ? "YES" : "NO");
    assert(n > 0 && n < 4096);

    result = calloc(1, sizeof(tpl) + (size_t)n);
    size   = (uint16_t)(sizeof(tpl) + (size_t)n - 4);
    store2(result, size);
    memcpy(result + sizeof(size), tpl + sizeof(size), sizeof(tpl) - sizeof(size));
    memcpy(result + sizeof(tpl),  buf, (size_t)n);

    result[0x03] = seq;
    return result;
}
