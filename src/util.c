#include <baro.h>
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

// Test strings for Unicode UTF8<->UTF32 conversions
struct test_case {
    char const *utf8;
    uint32_t utf32[];
} const
    ascii = {"abCD12!@", {'a', 'b', 'C', 'D', '1', '2', '!', '@', 0}},
    greek = {"κόσμε", {954, 972, 963, 956, 949, 0}},
    emoji = {"\xF0\x9F\x98\x8E\xF0\x9F\x98\xB8", {128526, 128568, 0}},
    *test_cases[] = {
        &ascii, &greek, &emoji,
};

TEST("[util] utf8_decode") {
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(struct test_case); i++) {
        char *str = (char *)test_cases[i]->utf8;

        for (size_t j = 0; test_cases[i]->utf32[j]; j++) {
            REQUIRE_EQ(utf8_decode(&str), test_cases[i]->utf32[j]);
        }
        REQUIRE_EQ(utf8_decode(&str), 0);
    }
}

size_t utf8_encode_partial(unsigned long code_point, size_t offset, char **buf, size_t len) {
    char encoded[4] = {0};
    unsigned long lead_byte_max = 0x7f, encoded_len = 0;

    while (code_point > lead_byte_max) {
        encoded[encoded_len++] = (char) ((code_point & 0x3fu) | 0x80u);
        code_point >>= 6u;
        lead_byte_max >>= (encoded_len == 1 ? 2u : 1u);
    }

    encoded[encoded_len++] = (char) ((code_point & lead_byte_max) | (~lead_byte_max << 1u));

    size_t index = encoded_len - offset, written = 0;
    while (index-- && written++ < len) {
        *(*buf)++ = encoded[index];
    }
    return written;
}

size_t utf8_encode(unsigned long code_point, char **buf, size_t len) {
    return utf8_encode_partial(code_point, 0, buf, len);
}

TEST("[util] utf8_encode") {
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(struct test_case); i++) {
        char *str = (char *)test_cases[i]->utf8;

        char buf[512] = {0};
        char *p = buf;
        size_t len = 512;
        for (size_t j = 0; test_cases[i]->utf32[j]; j++) {
            len -= utf8_encode(test_cases[i]->utf32[j], &p, len);
        }
        REQUIRE_STR_EQ(buf, str);
    }
}
