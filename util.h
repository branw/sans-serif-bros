#ifndef SSB_UTIL_H
#define SSB_UTIL_H

unsigned long utf8_decode(char **s);

void utf8_encode(char **s, char *end, unsigned long code);

#endif //SSB_UTIL_H
