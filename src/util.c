#include <stdlib.h>
#include "util.h"

unsigned long utf8_decode(char **s) {
    int k = **s ? __builtin_clz(~(**s << 24)) : 0;
    unsigned long mask = (unsigned) (1 << (8 - k)) - 1;
    unsigned long value = **s & mask;
    for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {
        value <<= 6;
        value += (**s & 0x3F);
    }
    return value;
}

size_t utf8_encode(size_t offset, char **buf, size_t len, unsigned long code_point) {
    char encoded[4] = {0};
    unsigned long lead_byte_max = 0x7f, encoded_len = 0;

    while (code_point > lead_byte_max) {
        encoded[encoded_len++] = (char) ((code_point & 0x3f) | 0x80);
        code_point >>= 6;
        lead_byte_max >>= encoded_len ? 2 : 1;
    }

    encoded[encoded_len++] = (char) ((code_point & lead_byte_max) | (~lead_byte_max << 1));

    size_t index = encoded_len - offset, written = 0;
    while (index-- && written++ < len) {
        *(*buf)++ = encoded[index];
    }
    return written;
}
