#include "state.h"
#include "session.h"

static bool title_screen_update(struct session *sess) {
    terminal_move(sess, 0, 0);
    terminal_write(sess, "hello\n\rworld\n\r");

    return true;
}

static bool playing_screen_update(struct session *sess) {
    return true;
}

static bool pause_screen_update(struct session *sess) {
    return true;
}

bool state_update(struct session *sess) {
    switch (sess->state.screen) {
    default:
    case title_screen:
        return title_screen_update(sess);

    case playing_screen:
        return playing_screen_update(sess);

    case pause_screen:
        return pause_screen_update(sess);
    }
}
