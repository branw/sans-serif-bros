#ifndef SSB_SERVER_H
#define SSB_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "session.h"

struct server {
    int socket;

    size_t num_sessions;
    struct session *sessions;
};

bool server_create(struct server *server, char *service);

void server_destroy(struct server *server);

bool server_update(struct server *server);

void server_disconnect_session(struct server *server, struct session *session);

bool server_next_session(struct server *server, struct session **session);

#ifdef __cplusplus
}
#endif

#endif //SSB_SERVER_H
