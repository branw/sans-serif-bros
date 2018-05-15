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

void utf8_encode(char **s, char *end, unsigned long code) {
    char val[4];
    int lead_byte_max = 0x7F, val_index = 0;
    while (code > lead_byte_max) {
        val[val_index++] = (char) ((code & 0x3F) | 0x80);
        code >>= 6;
        lead_byte_max >>= (val_index == 1 ? 2 : 1);
    }
    val[val_index++] = (char) ((code & lead_byte_max) | (~lead_byte_max << 1));
    while (val_index-- && *s < end) {
        **s = val[val_index];
        (*s)++;
    }
}
