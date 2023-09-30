#ifndef SSB_CONFIG_H
#define SSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Number of columns in the game field
#define COLUMNS 80
// Number of rows in the game field
#define ROWS 25

// Show incoming messages
#define DEBUG_INCOMING 1
// Show outgoing messages
#define DEBUG_OUTGOING 1

// Maximum number of concurrent sessions
#define MAX_SESSIONS 2

// Duration in ms between updates
#define TICK_DURATION 100

// Message displayed immediately upon connecting
#define WELCOME_MESSAGE \
    "Super Serif Bros. Telnet Edition\n\r" \
    "(commit " GIT_COMMIT_HASH " from " GIT_COMMIT_TIMESTAMP ")\n\r"

// Minimum level of log lines to display in stdout
#define MINIMUM_LOG_LEVEL LOG_LEVEL_DEBUG

#ifdef __cplusplus
}
#endif

#endif //SSB_CONFIG_H
