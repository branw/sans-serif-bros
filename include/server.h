#ifndef SSB_SERVER_H
#define SSB_SERVER_H

#include "session.h"

#ifdef __linux__
typedef int SOCKET;
#endif

struct server {
    SOCKET listen_sock;
    struct session *sessions;
    int num_sessions;
    int total_sessions;
};

bool server_create(struct server *server);

void server_accept_session(struct server *server);

bool server_next_session(struct server *server, struct session **sess);

void server_disconnect_session(struct server *server, struct session *sess);

void server_run(struct server *server);

#endif //SSB_SERVER_H
