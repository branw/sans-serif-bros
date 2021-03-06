#ifndef SSB_UTIL_H
#define SSB_UTIL_H

unsigned long utf8_decode(char **s);

size_t utf8_encode_partial(unsigned long code_point, size_t offset, char **buf, size_t len);

size_t utf8_encode(unsigned long code_point, char **buf, size_t len);

#define MIN(A, B) ((A) > (B) ? (B) : (A))

#endif //SSB_UTIL_H
