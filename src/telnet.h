#ifndef SSB_TELNET_H
#define SSB_TELNET_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESC "\x1b" // 13

#define ECHO "\x01" // 1
#define SUPPRESS_GO_AHEAD "\x03" // 3
#define NAWS "\x1f" // 31
#define TERMINAL_SPEED "\x20" // 32

#define CSI ESC "["

#define IAC  "\xff" // 255
#define DONT "\xfe" // 254
#define DO   "\xfd" // 253
#define WONT "\xfc" // 252
#define WILL "\xfb" // 251
#define SB   "\xfa" // 250
#define GA   "\xf9"
#define EL   "\xf8"
#define EC   "\xf7"
#define AYT  "\xf6"
#define AO   "\xf5"
#define IP   "\xf4"
#define BRK  "\xf3"
#define DM   "\xf2"
#define NOP  "\xf1"
#define SE   "\xf0"
#define EOR  "\xef"

#ifdef __cplusplus
}
#endif

#endif //SSB_TELNET_H
