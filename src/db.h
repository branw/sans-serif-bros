#ifndef SSB_DB_H
#define SSB_DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sqlite3.h>

struct metadata {
    // Number of the level, starting at 1 and not necessarily continuous,
    // considering the inclusion of recovered levels
    uint32_t id;
    // 12 UTF-8 character name of the level (worst case: 12 * 4 + 1)
    uint8_t name[49];

    // GMT Unix timestamp of the level creation time
    uint64_t creation_time;

    // Number of times the level has been started
    uint32_t num_attempts;
    // Number of winning plays
    uint32_t num_wins;
    // Number of deaths
    uint32_t num_deaths;

    // Lowest number of game ticks taken to win the level
    uint32_t min_ticks;
    // Average number of game ticks to win the level
    uint32_t average_ticks;
};

struct level {
    // Level metadata
    struct metadata metadata;
    // Lazy-loaded field of 80x25 UTF-32 characters
    uint32_t *field;
};

struct db {
    sqlite3 *db;
};

bool db_create(struct db *db, char *path, char *levels_path);

void db_destroy(struct db *db);

int db_get_metadata(struct db *db, uint32_t id, struct metadata *metadata, int count);

int db_get_previous_level(struct db *db, uint32_t before_id);

int db_num_levels(struct db *db);

bool db_get_level_bounds(struct db *db, uint32_t *min_level, uint32_t *max_level);

bool db_get_level_field_utf8(struct db *db, uint32_t id, char **field);

bool db_create_level_utf8(struct db *db, char *name, char *field, struct metadata *metadata);

enum game_state;

bool db_insert_attempt(struct db *db, uint32_t level_id, uint32_t ticks, enum game_state game_state, char *input_log);

#ifdef __cplusplus
}
#endif

#endif //SSB_DB_H
