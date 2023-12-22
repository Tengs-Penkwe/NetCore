#ifndef __LOG_H__
#define __LOG_H__

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>     //strrchr

// Log Level Implementation for Debug Prints in Barrelfish OS
// In order to facilitate various levels of logging such as info, debug, and error, a log level system can be implemented. This system allows developers to specify a log level for each message and configure the system to only output messages at or above a certain level.
enum log_level {
    LOG_LEVEL_TRIVIAL = 0,       // Unimportant
    LOG_LEVEL_DEBUG = 1,         // Debug messages
    LOG_LEVEL_INFO = 2,          // Informational messages
    LOG_LEVEL_WARN = 3,          // Warning messages
    LOG_LEVEL_ERROR = 4,         // Critical errors
    LOG_LEVEL_NONE = 5,          // No logs
};
// Current Log Level Setting
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

#define __BASEFILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Function Prototypes
void log_printf(enum log_level level, int line, const char* func, const char* file, const char *msg, ...);

#define LOG_PRINT(level, fmt, ...) \
    do { \
        if(level >= CURRENT_LOG_LEVEL) { \
            log_printf(level, __LINE__, __func__, __BASEFILE__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_TRIVIAL(fmt, ...) LOG_PRINT(LOG_LEVEL_TRIVIAL, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   LOG_PRINT(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    LOG_PRINT(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    LOG_PRINT(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)     LOG_PRINT(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#endif  //__LOG_H__
