#ifndef SSB_UTIL_H
#define SSB_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// Reimplementation of MIN/MAX to avoid stomping over MSVC's definition

#define SSB_MIN(A, B) ((A) > (B) ? (B) : (A))
#define SSB_MAX(A, B) ((A) < (B) ? (B) : (A))
#define SSB_CLAMP(X, LOWER, UPPER) SSB_MAX(SSB_MIN((X), (UPPER)), (LOWER))

unsigned long utf8_decode(char **s);

size_t utf8_encode_partial(unsigned long code_point, size_t offset, char **buf, size_t len);

size_t utf8_encode(unsigned long code_point, char **buf, size_t len);

char const *kmp_strnstr(char const *haystack, char const *needle, size_t haystack_len);

#ifdef __cplusplus
}
#endif

#endif //SSB_UTIL_H
