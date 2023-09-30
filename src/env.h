#ifndef SSB_ENV_H
#define SSB_ENV_H

#ifdef __cplusplus
extern "C" {
#endif

struct env {
    struct server *server;
    struct db *db;
};

#ifdef __cplusplus
}
#endif

#endif //SSB_ENV_H
