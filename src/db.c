#include <memory.h>
#include <dirent.h>
#include <sqlite3.h>
#include <errno.h>
#include "db.h"
#include "util.h"
#include "config.h"
#include "game.h"
#include "log.h"

static int get_user_version(sqlite3 *db) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        LOG_ERROR("get_user_version prepare failed: %d", rc);
        return -1;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        LOG_ERROR("get_user_version step failed: %d", rc);
        return -1;
    }

    int user_version = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return user_version;
}

// Executes a multi-statement SQL query, without grabbing any results
static bool execute_many_statements(sqlite3 *db, char const *sql) {
    char const *next_sql = sql;
    while (next_sql != NULL && *next_sql != '\0') {
        sqlite3_stmt *stmt = NULL;
        char const *tail = NULL;
        int rc = sqlite3_prepare_v2(db, next_sql, -1, &stmt, &tail);
        LOG_DEBUG("Running query \"%.*s\"", (int)(tail - next_sql), next_sql);
        next_sql = tail;
        if (rc != SQLITE_OK) {
            LOG_ERROR("execute_many_statements prepare failed: %s (%d)", sqlite3_errmsg(db), rc);
            return false;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOG_ERROR("execute_many_statements step failed: %d", rc);
            return false;
        }

        sqlite3_finalize(stmt);
    }

    return true;
}

#define CURRENT_VERSION 1

static bool read_and_validate_level(char *path, char **field_str) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        LOG_ERROR("load_levels fopen \"%s\" failed: %d", path, errno);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);

    *field_str = malloc(len + 1);
    if (field_str == NULL) {
        LOG_ERROR("load_levels malloc failed: %d", errno);
        fclose(f);
        return false;
    }

    size_t len_read = fread(*field_str, sizeof(char), len, f);
    if (len_read != len) {
        LOG_ERROR("fread failed for \"%s\"", path);
        free(*field_str);
        fclose(f);
        return false;
    }
    (*field_str)[len_read] = '\0';

    fclose(f);

    uint32_t field[ROWS][COLUMNS] = {0};
    if (!game_parse_and_validate_field(*field_str, (uint32_t *) field)) {
        LOG_ERROR("Field validation failed for \"%s\"", path);
        free(*field_str);
        return false;
    }
    return true;
}

static bool load_levels(sqlite3 *db, char *levels_path) {
    DIR *d = opendir(levels_path);
    if (d == NULL) {
        return false;
    }

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, "INSERT INTO level (id, name, field) VALUES (?, ?, ?);", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("load_levels prepare failed: %d", rc);
        return false;
    }

    size_t const levels_path_len = strlen(levels_path);
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        // We only care about regular files
        if (dir->d_type != DT_REG) {
            continue;
        }

        // with a .txt extension
        char const *file_name = dir->d_name;
        char const *ext = strrchr(file_name, '.');
        if (ext == NULL || ext == file_name || strcmp(ext, ".txt") != 0) {
            continue;
        }

        char *end_ptr = NULL;
        int id = (int)strtol(file_name, &end_ptr, 10);
        if (end_ptr != ext) {
            LOG_ERROR("Skipping \"%s\" because it has an invalid file name", file_name);
            continue;
        }

        size_t path_len = levels_path_len + 1 + strlen(file_name) + 1;
        char *path = malloc(path_len);
        snprintf(path, path_len, "%s/%s", levels_path, file_name);

        char *field_str = NULL;
        if (!read_and_validate_level(path, &field_str)) {
            LOG_ERROR("Failed to load \"%s\"", path);
            continue;
        }

        char name[13] = {0};
        snprintf(name, 12, "%d", id);

        rc = sqlite3_bind_int(stmt, 1, id);
        if (rc != SQLITE_OK) {
            LOG_ERROR("load_levels bind id failed: %d", rc);
            free(path);
            closedir(d);
            return false;
        }

        rc = sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            LOG_ERROR("load_levels bind name failed: %d", rc);
            free(path);
            closedir(d);
            return false;
        }

        rc = sqlite3_bind_text(stmt, 3, field_str, -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            LOG_ERROR("load_levels bind field_str failed: %d", rc);
            free(path);
            closedir(d);
            return false;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            LOG_ERROR("load_levels step failed: %d", rc);
            free(path);
            closedir(d);
            return false;
        }

        sqlite3_reset(stmt);

        free(path);

    }
    closedir(d);

    sqlite3_finalize(stmt);

    return true;
}

// Safely migrates a DB to the latest version
static bool migrate(sqlite3 *db, char *levels_path) {
    int user_version = get_user_version(db);
    if (user_version == -1) {
        LOG_ERROR("Failed to get DB version");
        return false;
    }

    if (user_version == CURRENT_VERSION) {
        LOG_INFO("DB is already the latest version (%d)", user_version);
        return true;
    } else if (user_version > CURRENT_VERSION) {
        LOG_ERROR("DB is at a future version (latest version is %d, but the database is version %d)",
                user_version, CURRENT_VERSION);
        return false;
    }

    // Only load levels for fresh databases
    bool should_load_levels = false;

    switch (user_version) {
        // Create a level table
        case 0:
            should_load_levels = true;
            LOG_DEBUG("Migrating DB from version 0 to 1");
            execute_many_statements(db,
                                    "BEGIN;"
                                    "CREATE TABLE level ("
                                    "    id INTEGER NOT NULL PRIMARY KEY,"
                                    "    name TEXT NOT NULL,"
                                    "    field TEXT NOT NULL,"
                                    "    creation_timestamp INTEGER NOT NULL DEFAULT CURRENT_TIMESTAMP);"
                                    "PRAGMA user_version = 1;"
                                    "COMMIT;");
            // fallthrough

        case CURRENT_VERSION:
            if (should_load_levels) {
                return load_levels(db, levels_path);
            }
            return true;

        default:
            LOG_ERROR("Unknown user version %d", user_version);
            return false;
    }
}

bool db_create(struct db *db, char *path, char *levels_path) {
    int rc = sqlite3_open(path, &db->db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("sqlite3_open failed: %d", rc);
        return false;
    }

    if (!migrate(db->db, levels_path)) {
        LOG_ERROR("DB migration failed");

        sqlite3_close(db->db);
        return false;
    }

    return true;
}

void db_destroy(struct db *db) {
    sqlite3_close(db->db);
}

// Get metadata for up to `count` levels, starting after `id`
int db_get_metadata(struct db *db, uint32_t after_id, struct metadata *metadata, int count) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "SELECT id, name, creation_timestamp FROM level WHERE id > ? LIMIT ?;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_metadata prepare failed: %d", rc);
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 1, (int) after_id);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_metadata bind 1 failed: %d", rc);
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 2, count);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_metadata bind 2 failed: %d", rc);
        return 0;
    }

    int num_levels = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        uint8_t const *name = sqlite3_column_text(stmt, 1);
        size_t name_len = sqlite3_column_bytes(stmt, 1);
        int creation_timestamp = sqlite3_column_int(stmt, 2);

        struct metadata m = {
                .id = id,
                .creation_time = creation_timestamp,
        };

        uint32_t *m_name = m.name;
        while (name_len-- && m_name++) {
            *m_name = utf8_decode((char **)&name);
        }

        metadata[num_levels] = m;

        num_levels++;
    }
    if (rc != SQLITE_DONE) {
        LOG_ERROR("db_get_metadata not done: %d", rc);
        return 0;
    }

    sqlite3_finalize(stmt);

    return num_levels;
}

int db_get_previous_level(struct db *db, uint32_t before_id) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "SELECT id FROM level WHERE id < ? ORDER BY id DESC LIMIT 1;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_previous_level prepare failed: %d", rc);
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 1, (int) before_id);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_previous_level bind failed: %d", rc);
        return 0;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        return 0;
    } else if (rc != SQLITE_ROW) {
        LOG_ERROR("db_get_previous_level step failed: %d", rc);
        return 0;
    }

    int previous_level = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return previous_level;
}

int db_num_levels(struct db *db) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "SELECT COUNT(*) FROM level;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_num_levels prepare failed: %d", rc);
        return 0;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        LOG_ERROR("db_num_levels step failed: %d", rc);
        return 0;
    }

    int num_levels = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return num_levels;
}

bool db_get_level_bounds(struct db *db, uint32_t *min_level, uint32_t *max_level) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "SELECT MIN(id), MAX(id) FROM level;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_level_bounds prepare failed: %d", rc);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        LOG_ERROR("db_get_level_bounds step failed: %d", rc);
        return false;
    }

    *min_level = sqlite3_column_int(stmt, 0);
    *max_level = sqlite3_column_int(stmt, 1);

    sqlite3_finalize(stmt);

    return true;
}

// Caller is responsible for freeing field
bool db_get_level_field_utf8(struct db *db, uint32_t id, char **field) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "SELECT field FROM level WHERE id = ?;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_level_field_utf8 prepare failed: %d", rc);
        return false;
    }

    rc = sqlite3_bind_int(stmt, 1, (int)id);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_get_level_field_utf8 bind failed: %d", rc);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        LOG_ERROR("Failed to find level %d", id);
        return false;
    } else if (rc != SQLITE_ROW) {
        LOG_ERROR("db_get_level_field_utf8 step failed: %d", rc);
        return false;
    }

    uint8_t const *data = sqlite3_column_text(stmt, 0);
    size_t const len = sqlite3_column_bytes(stmt, 0);

    *field = malloc(len + 1);
    if (*field == NULL) {
        LOG_ERROR("db_get_level_field_utf8 malloc failed: %d", errno);
        return false;
    }
    memcpy(*field, data, len);
    (*field)[len] = '\0';

    sqlite3_finalize(stmt);

    return true;
}

bool db_create_level_utf8(struct db *db, char *name, char *field, struct metadata *metadata) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db->db, "INSERT INTO level (name, field) VALUES (?, ?) RETURNING id, creation_timestamp;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_create_level_utf8 prepare failed: %d", rc);
        return false;
    }

    rc = sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        LOG_ERROR("db_create_level_utf8 bind name failed: %d", rc);
        return false;
    }

    //TODO get field into a better form
//    rc = sqlite3_bind_text(stmt, 2, field_string, -1, SQLITE_TRANSIENT);
//    if (rc != SQLITE_OK) {
//        LOG_ERROR("db_create_level_utf8 bind field failed: %d", rc);
//        return false;
//    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        LOG_ERROR("db_create_level_utf8 step failed: %d", rc);
        return false;
    }

    //TODO build the level using the returned data



    return true;
}
