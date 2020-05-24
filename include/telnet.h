#ifndef SSB_TELNET_H
#define SSB_TELNET_H

#define ESC "\x1b"

#define ECHO "\x01"
#define SUPPRESS_GO_AHEAD "\x03"
#define NAWS "\x1f"

#define CSI ESC "["

#define IAC  "\xff"
#define DONT "\xfe"
#define DO   "\xfd"
#define WONT "\xfc"
#define WILL "\xfb"
#define SB   "\xfa"
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

#endif //SSB_TELNET_H
