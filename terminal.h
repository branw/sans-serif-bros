#ifndef SSB_TERMINAL_H
#define SSB_TERMINAL_H

struct session;

enum parse_state {
    parse_data, parse_iac, parse_will, parse_wont, parse_do, parse_dont, parse_sb, parse_sb_data,
    parse_sb_data_iac
};

void terminal_parse(struct session *sess, char *buf, int len);

void terminal_write(struct session *sess, char *buf);

void terminal_move(struct session *sess, int x, int y);

#endif //SSB_TERMINAL_H
