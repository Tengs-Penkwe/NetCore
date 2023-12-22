#include <log.h>
#include <stdio.h>

enum log_level log_matrix[LOG_MODULE_COUNT] = {
#define X(module, level) level,
    LOG_MODULE_LEVELS
#undef X
};

// Utility function to convert log level enum to string
const char* level_to_string(enum log_level level) {
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
const char* module_to_string(enum log_module module) {
    switch (module) {
        case LOG:       return "LOG";
        case DRIVER:    return "DRIVER";
        case ETHER:     return "ETHER";
        case ARP:       return "ARP";
        case IP:        return "IP";
        case ICMP:      return "ICMP";
        case UDP:       return "UDP";
        case TCP:       return "TCP";
        default:        return "UNKNOWN_MODULE";
    }
}

void log_printf(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...)
{
    va_list ap;
    static char buffer[1024]; 

    const char *leader = level < LOG_LEVEL_NONE ? level_colors[level] : "Unknown";

    // Format the log prefix with level, module, file, line, and function
    int offset = snprintf(buffer, sizeof(buffer), "%s[%s-%s] %s:%d:%s(): ", leader, level_to_string(level), module_to_string(module), file, line, func);

    // Append the formatted message
    if (msg != NULL && offset < (int)sizeof(buffer)) {
        va_start(ap, msg);
        vsnprintf(buffer + offset, sizeof(buffer) - offset, msg, ap);
        va_end(ap);
    }

    // Reset color at the end of the message
    const char *tail = level < LOG_LEVEL_INFO ? "\n" : "\x1B[0m\n";
    strncat(buffer, tail, sizeof(buffer) - strlen(buffer) - 1);

    // Print the log message
    printf("%s", buffer);
}
