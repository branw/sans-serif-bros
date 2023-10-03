#ifndef SSB_LOG_H
#define SSB_LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum log_level {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

void log_push_context(uint64_t session_id);
void log_pop_context(void);

void log_printf(enum log_level level, char const *file, int line, char const *func, char const *format, ...);

#define LOG_TRACE(...) log_printf(LOG_LEVEL_TRACE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG(...) log_printf(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO(...) log_printf(LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_WARN(...) log_printf(LOG_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_ERROR(...) log_printf(LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_FATAL(...) log_printf(LOG_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif //SSB_LOG_H
