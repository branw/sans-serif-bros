#include "state.h"
#include "session.h"

void state_init(struct session *sess) {
    terminal_send(sess, IAC WILL ECHO, 3);
    terminal_send(sess, IAC DONT ECHO, 3);
    terminal_send(sess, IAC WILL SUPPRESS_GO_AHEAD, 3);

    terminal_write(sess, "Super Serif Bros. Telnet Edition\n\r"
                         "(commit " GIT_COMMIT_HASH " from " GIT_COMMIT_TIMESTAMP ")\n\r");

    terminal_clear(sess);
    terminal_reset(sess);
    terminal_cursor(sess, false);

    sess->state.screen = title_screen;
}


static bool title_screen_update(struct session *sess) {
    terminal_move(sess, 0, 0);
    terminal_rect(sess, 0, 0, 80, 25, '#');
    //terminal_write(sess,);

    terminal_move(sess, 25, 11);
    terminal_write(sess, "hello, ");
    terminal_inverse(sess, true);
    terminal_write(sess, "world!");
    terminal_inverse(sess, false);

    return true;
}


bool state_update(struct session *sess) {
    switch (sess->state.screen) {
    default:
    case title_screen:
        return title_screen_update(sess);
    }
}
