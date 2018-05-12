#ifndef SSB_SESSION_H
#define SSB_SESSION_H

#include <Winsock2.h>
#include <stdbool.h>
#include <time.h>

struct session {
    SOCKET client_sock;
    struct timespec last_tick;

    struct session *prev;
    struct session *next;
};

bool session_create(struct session *session, SOCKET client_sock);

void session_shutdown(struct session *sess);

bool session_update(struct session *sess);

#endif //SSB_SESSION_H
