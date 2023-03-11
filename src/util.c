#include <baro.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

static int *kmp_borders(const char *needle, size_t len) {
    if (needle == NULL) {
        return NULL;
    }

    int *borders = malloc((len+1) * sizeof(*borders));
    if (borders == NULL) {
        return NULL;
    }

    size_t i = 0;
    int j = -1;
    borders[i] = j;

    while (i < len) {
        while (j >= 0 && needle[i] != needle[j]) {
            j = borders[j];
        }
        ++i;
        ++j;
        borders[i] = j;
    }

    return borders;
}

static char const *kmp_search(char const *haystack, size_t haystack_len,
                        const char *needle, size_t needle_len, const int *borders){
    size_t max_index = haystack_len - needle_len, i = 0, j = 0;
    while (i <= max_index) {
        while (j < needle_len && *haystack && needle[j] == *haystack){
            ++j;
            ++haystack;
        }
        if (j == needle_len) {
            return haystack - needle_len;
        }
        if (*haystack == '\0') {
            return NULL;
        }
        if (j == 0) {
            ++haystack;
            ++i;
        } else {
            do {
                i += j - (size_t)borders[j];
                j = borders[j];
            } while (j > 0 && needle[j] != *haystack);
        }
    }
    return NULL;
}

// Knuth-Morris-Pratt (KMP) string search. Like strstr, but O(m + n) and it
// can handle non-NUL-terminated strings. Portable equivalent of BSD's strnstr.
char const *kmp_strnstr(char const *haystack, char const *needle, size_t haystack_len) {
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }

    size_t needle_len = strlen(needle);
    if (haystack_len < needle_len) {
        return NULL;
    }

    int *borders = kmp_borders(needle, needle_len);
    if (borders == NULL) {
        return NULL;
    }

    char const *match = kmp_search(haystack, haystack_len, needle, needle_len, borders);

    free(borders);
    return match;
}

TEST("[util] kmp_strnstr") {
    char const *haystack1 = "foobarbaz";
    size_t const haystack1_len = 9;

    REQUIRE_EQ(kmp_strnstr(haystack1,  "foo", haystack1_len), haystack1);
    REQUIRE_EQ(kmp_strnstr(haystack1,  "o", haystack1_len), &haystack1[1]);
    REQUIRE_EQ(kmp_strnstr(haystack1,  "bar", haystack1_len), &haystack1[3]);
    REQUIRE_EQ(kmp_strnstr(haystack1,  "z", haystack1_len), &haystack1[8]);
    REQUIRE_EQ(kmp_strnstr(haystack1,  "bazinga", haystack1_len), NULL);

    REQUIRE_EQ(kmp_strnstr(haystack1, NULL, haystack1_len), NULL);
    REQUIRE_EQ(kmp_strnstr(haystack1, "foo", 1), NULL);
}