#include <stdlib.h>
#include <stdint.h>
#include "util.h"

unsigned long utf8_decode(char **s) {
    int k = **s ? __builtin_clz(~((uint32_t)**(unsigned char **)s << 24u)) : 0;
    unsigned long mask = (unsigned) (1u << (8u - k)) - 1;
    unsigned long value = **(unsigned char **)s & mask;
    for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {
        value <<= 6u;
        value += (**(unsigned char **)s & 0x3Fu);
    }
    return value;
}

size_t utf8_encode(size_t offset, char **buf, size_t len, unsigned long code_point) {
    char encoded[4] = {0};
    unsigned long lead_byte_max = 0x7f, encoded_len = 0;

    while (code_point > lead_byte_max) {
        encoded[encoded_len++] = (char) ((code_point & 0x3fu) | 0x80u);
        code_point >>= 6u;
        lead_byte_max >>= encoded_len ? 2u : 1u;
    }

    encoded[encoded_len++] = (char) ((code_point & lead_byte_max) | (~lead_byte_max << 1u));

    size_t index = encoded_len - offset, written = 0;
    while (index-- && written++ < len) {
        *(*buf)++ = encoded[index];
    }
    return written;
}
