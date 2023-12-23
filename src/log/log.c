#include <log.h>
#include <stdio.h>
#include <pthread.h>

enum log_level log_matrix[LOG_MODULE_COUNT] = {
#define X(module, level) level,
    LOG_MODULE_LEVELS
#undef X
};

// Utility function to convert log level enum to string
static const char* level_to_string(enum log_level level) {
    switch (level) {
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        case LOG_LEVEL_INFO:    return "INFO ";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_NOTE:    return "NOTE ";
        case LOG_LEVEL_WARN:    return "WARN ";
        case LOG_LEVEL_ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

const char *level_colors[] = {
    [LOG_LEVEL_VERBOSE] = "", 
    [LOG_LEVEL_INFO]    = "\x1B[1m", 
    [LOG_LEVEL_DEBUG]   = "\x1B[0;34m", 
    [LOG_LEVEL_NOTE]    = "\x1B[1;34m", // Blue
    [LOG_LEVEL_WARN]    = "\x1B[0;33m", // Yellow
    [LOG_LEVEL_ERROR]   = "\x1B[0;91m"  // Red
};

// Utility function to convert log module enum to string
static const char* module_to_string(enum log_module module) {
    switch (module) {
        case LOG:              return "LOG";
        case MODULE_DEVICE:    return "DEVICE";
        case MODULE_ETHER:     return "ETHER";
        case MODULE_ARP:       return "ARP";
        case MODULE_IP:        return "IP";
        case MODULE_ICMP:      return "ICMP";
        case MODULE_UDP:       return "UDP";
        case MODULE_TCP:       return "TCP";
        default:        return "UNKNOWN_MODULE";
    }
}

void log_printf(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...)
{
    static char buffer[1024]; 
    const char *leader = level < LOG_LEVEL_NONE ? level_colors[level] : "Unknown";

    pthread_t thread_id = pthread_self();
    // Format the log prefix with level, module, file, line, and function
    int len = snprintf(buffer, sizeof(buffer), "%s[%s-%s]<%u> %s:%d->%s(): ", leader, level_to_string(level), module_to_string(module), thread_id, file, line, func);

    // Append the formatted message
    if (msg != NULL && len < (int)sizeof(buffer)) {
        va_list ap;
        va_start(ap, msg);
        len += vsnprintf(buffer + len, sizeof(buffer) - len, msg, ap);
        va_end(ap);
    }

    // Reset color at the end of the message
    const char *tail = level < LOG_LEVEL_INFO ? "\n" : "\x1B[0m\n";
    len += snprintf(buffer + len, sizeof(buffer) - len, tail);

    sys_print(buffer, len);
}
