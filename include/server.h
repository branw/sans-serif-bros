#ifndef SSB_SERVER_H
#define SSB_SERVER_H

#include "session.h"

#ifdef __linux__
#ifndef SOCKET
typedef int SOCKET;
#endif
#endif

struct server {
    SOCKET socket;

    size_t num_sessions;
    struct session *sessions;
};

bool server_create(struct server *server, char *service);

void server_destroy(struct server *server);

bool server_update(struct server *server);

void server_disconnect_session(struct server *server, struct session *session);

bool server_next_session(struct server *server, struct session **session);

#endif //SSB_SERVER_H
