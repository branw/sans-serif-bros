#ifndef SSB_SESSION_H
#define SSB_SESSION_H

#include "state.h"

struct session {
    int socket;

    struct state *state;

    struct session *prev;
    struct session *next;
};

bool session_create(struct session *session, int socket);

void session_destroy(struct session *session);

bool session_receive(struct session *session, char *buf, size_t len, size_t *len_written);

void session_send(struct session *session, char *buf, size_t len);

#endif //SSB_SESSION_H
