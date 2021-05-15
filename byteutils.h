#ifndef BYTEUTILS_H
#define BYTEUTILS_H

#include <stdint.h>
#include <string.h>

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_BIG_ENDIAN__) || !defined(__ORDER_LITTLE_ENDIAN__)
#error "__BYTE_ORDER__ or __ORDER_BIG_ENDIAN__ or __ORDER_LITTLE_ENDIAN__ macro is not defined."
#endif

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ && __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error "Unsupported __BYTE_ORDER__"
#endif

static inline uint8_t load1(const unsigned char* buf)
{
    return (uint8_t)(*buf);
}

static inline uint16_t load2(const unsigned char* buf)
{
    uint16_t result;
    memcpy(&result, buf, 2);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ((result >> 8) & 0xFFu) | ((result & 0xFFu) << 8);
#endif
    return result;
}

static inline uint32_t load3(const unsigned char* buf)
{
    uint32_t result = 0;
    memcpy(&result, buf, 3);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return 
        ((result & 0xFF000000u) >> 24) |
        ((result & 0x00FF0000u) >>  8) |
        ((result & 0x0000FF00u) <<  8) |
        ((result & 0x000000FFu) << 24)
    ;
#endif
    return result;
}

static inline uint32_t load4(const unsigned char* buf)
{
    uint32_t result;
    memcpy(&result, buf, 4);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return 
        ((result & 0xFF000000u) >> 24) |
        ((result & 0x00FF0000u) >>  8) |
        ((result & 0x0000FF00u) <<  8) |
        ((result & 0x000000FFu) << 24)
    ;
#endif
    return result;
}

static inline uint64_t load8(const unsigned char* buf)
{
    uint64_t result = 0;
    memcpy(&result, buf, 8);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return 
        ((result & 0xFF00000000000000ull) >> 56) |
        ((result & 0x00FF000000000000ull) >> 40) |
        ((result & 0x0000FF0000000000ull) >> 24) |
        ((result & 0x000000FF00000000ull) >>  8) |
        ((result & 0x00000000FF000000ull) <<  8) |
        ((result & 0x0000000000FF0000ull) << 24) |
        ((result & 0x000000000000FF00ull) << 40) |
        ((result & 0x00000000000000FFull) << 56);
#endif
    return result;
}

#endif
