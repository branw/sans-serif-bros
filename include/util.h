#ifndef SSB_UTIL_H
#define SSB_UTIL_H

unsigned long utf8_decode(char **s);

size_t utf8_encode(size_t offset, char **buf, size_t len, unsigned long code_point);

#endif //SSB_UTIL_H
