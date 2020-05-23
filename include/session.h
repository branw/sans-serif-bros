#ifndef SSB_SESSION_H
#define SSB_SESSION_H

#ifdef _WIN32
#include <Winsock2.h>
#include <stdbool.h>

#endif

#ifdef __linux__
typedef int SOCKET;
#endif

#include "state.h"

struct session {
    SOCKET client_sock;
    struct state state;

    int id;
    struct session *prev;
    struct session *next;
};

bool session_create(struct session *sess, SOCKET client_sock);

void session_shutdown(struct session *sess);

bool session_update(struct session *sess);

#endif //SSB_SESSION_H
