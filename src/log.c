#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include "log.h"
#include "config.h"

struct log_line {
    struct tm *time;
    enum log_level level;

    char const *file;
    int line;
    char const *func;

    char const *format;
    va_list ap;
};

static const char *level_strings[6] = {
        [LOG_LEVEL_TRACE] = "TRC",
        [LOG_LEVEL_DEBUG] = "DBG",
        [LOG_LEVEL_INFO] =  "INF",
        [LOG_LEVEL_WARN] =  "WRN",
        [LOG_LEVEL_ERROR] = "ERR",
        [LOG_LEVEL_FATAL] = "!!!",
};
static const char *level_colors[6] = {
        [LOG_LEVEL_TRACE] = "\x1b[94m", // Bright blue foreground
        [LOG_LEVEL_DEBUG] = "\x1b[32m", // Green foreground
        [LOG_LEVEL_INFO] =  "\x1b[36m", // Cyan foreground
        [LOG_LEVEL_WARN] =  "\x1b[43m", // Yellow background
        [LOG_LEVEL_ERROR] = "\x1b[41m", // Red background
        [LOG_LEVEL_FATAL] = "\x1b[40m\x1b[37m", // Black background, white foreground
};

// ID of the currently executing session
_Atomic uint64_t ctx_session_id = 0;

// Handle to current log file
_Atomic FILE *log_file = 0;
// Creation timestamp of the current log file (aligned to the previous hour)
_Atomic uint64_t last_log_rotation = 0;

void log_push_context(uint64_t session_id) {
    if (ctx_session_id != 0) {
        LOG_ERROR("Attempted to push new context (%u) when one already exists (%u)", ctx_session_id, session_id);
    } else {
        ctx_session_id = session_id;
    }
}

void log_pop_context(void) {
    ctx_session_id = 0;
}

static void print_log_line(FILE *f, struct log_line *log_line) {
    char buf[16] = {0};
    size_t len = strftime(buf, sizeof(buf), "%H:%M:%S", log_line->time);
    buf[len] = '\0';

    // Show info from the current logging context
    char ctx_info[16] = {0};
    if (ctx_session_id) {
        snprintf(ctx_info, sizeof(ctx_info), " (#%llu)", ctx_session_id);
    }

    fprintf(f, "%s %s%s\x1b[0m \x1b[90m%s:%d:%s%s:\x1b[0m ",
            buf, level_colors[log_line->level], level_strings[log_line->level],
            log_line->file, log_line->line, log_line->func, ctx_info);
    vfprintf(f, log_line->format, log_line->ap);
    fprintf(f, "\n");
    fflush(f);
}

void log_printf(enum log_level level, char const *file, int line, char const *func, char const *format, ...) {
    if (level < MINIMUM_LOG_LEVEL) {
        return;
    }

    time_t const t = time(NULL);
    struct log_line log_line = {
            .time = gmtime(&t),
            .level = level,

            .file = file,
            .line = line,
            .func = func,

            .format = format,
    };

    va_start(log_line.ap, format);

    print_log_line(stdout, &log_line);

    va_end(log_line.ap);
}
