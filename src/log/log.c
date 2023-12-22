#include <log/log.h>
#include <stdio.h>

void log_printf(enum log_level level, int line, const char* func, const char* file, const char *msg,  ...) 
{
    (void) level;
    (void) line;
    (void) func;
    (void) file;
    (void) msg;
    // va_list ap;
    // char    str[256];

    // const char *level_colors[] = {
    //     [LOG_LEVEL_TRIVIAL] = "[Trivial]", 
    //     [LOG_LEVEL_DEBUG]   = "\x1B[1m[Debug]", 
    //     [LOG_LEVEL_INFO]    = "\x1B[0;34m[Info]", // Blue
    //     [LOG_LEVEL_WARN]    = "\x1B[1;33m[Warn]", // Yellow
    //     [LOG_LEVEL_ERROR]   = "\x1B[1;91m[Error]"  // Red
    // };

    // const char *leader = level < LOG_LEVEL_NONE ? level_colors[level] : "Unknown";

    // size_t len = snprintf(str, sizeof(str), "%s(%.*s.%u.%u) %d:%s()->%s: ", leader, DISP_NAME_LEN,
    //                disp_name(), disp_get_current_core_id(), disp_get_domain_id(), line, func, file, leader);
    // if (msg != NULL) {
    //     va_start(ap, msg);
    //     len += vsnprintf(str + len, sizeof(str) - len, msg, ap);
    //     va_end(ap);
    // }

    // const char *tail = level < LOG_LEVEL_DEBUG ? "\n" : "\x1B[0m\n";

    // len += snprintf(str + len, sizeof(str) - len, tail);
    // printf(str);
}

