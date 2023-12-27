#include <errors/log.h>
#include <stdio.h>
#include <event/states.h>

errval_t log_init(const char* log_file, int log_level, FILE** ret_file) {

    FILE *log = fopen(log_file, "w");
    if (log == NULL) {
        char error[128];
        sprintf(error, "Error opening file: %s, going to use the standard output", log_file);
        perror(error);
    } 
    log = stdout;

    // Set full buffer mode, size as 65536
    setvbuf(log, NULL, _IOFBF, 65536);
    // TODO: read it from global state ?
    (void) log_level;

    return SYS_ERR_OK;
}

void log_close(FILE* log) {
    fflush(log);
    // Let the OS close the file to avoid race condition
    // fclose(log);
}

enum log_level log_matrix[LOG_MODULE_COUNT] = {
#define X(module, level) level,
    LOG_MODULE_LEVELS
#undef X
};

// Utility function to convert log level enum to string
static const char* level_to_string(enum log_level level) {
    switch (level) {
        case LOG_LEVEL_VERBOSE: return "VERBS";
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
        case MODULE_IPC:       return "IPC";
        case MODULE_EVENT:     return "EVENT";
        case MODULE_TIMER:     return "TIMER";
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
    char buffer[256]; 
    const char *leader = level < LOG_LEVEL_NONE ? level_colors[level] : "Unknown";

    LocalState *local = get_local_state();
    const char *name  = local->my_name;
    FILE       *log   = local->log_file;

    // Format the log prefix with level, module, file, line, and function
    int len = snprintf(buffer, sizeof(buffer), "%s[%s-%s]<%s> %s:%d->%s(): ", leader, level_to_string(level), module_to_string(module), name, file, line, func);

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

    fwrite(buffer, sizeof(char), len, log);
}

/*
 * Copyright (c) 2008-2011, ETH Zurich.
 * Copyright (c) 2015, 2016 Hewlett Packard Enterprise Development LP.
 * Copyright (c) 2022, The University of British Columbia.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <errors/debug.h>

#include <stdarg.h>         //va_list, va_start, va_end
#include <stdio.h>          //snprintf
#include <event/states.h>   //LocalState


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Debug prints
//
////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints out the message with the location and error information
 *
 * @param[in] file  Source file of the caller
 * @param[in] func  Function name of the caller
 * @param[in] line  Line number of the caller
 * @param[in] err   error number
 * @param[in] msg   Message format to be printed (printf style)
 *
 * @note: do not use this function directly. Rather use the DEBUG_ERR macro
 */
void debug_err(const char *file, const char *func, int line, errval_t err, const char *msg, ...)
{
    char    str[256];
    size_t  len;

    char *leader = err_is_ok(err) ? "\x1B[1;32mSUCCESS" : "\x1B[1;91mERROR";

    LocalState *local = get_local_state();
    const char *name  = local->my_name;
    FILE       *log   = local->log_file;

    len = snprintf(str, sizeof(str), "%s<%s>:%s() %s:%d\n%s: ", leader, name, func, file, line, leader);
    if (msg != NULL) {
        va_list ap;
        va_start(ap, msg);
        len += vsnprintf(str + len, sizeof(str) - len, msg, ap);
        va_end(ap);
    }

    len += snprintf(str + len, sizeof(str) - len, "\x1B[0m\n");
    fwrite(str, sizeof(char), len, log);

    if (err_is_fail(err)) {
        leader = "Error calltrace:\n";
        fwrite(leader, sizeof(char), strlen(leader), log);
        err_print_calltrace(err, log);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PANIC
//
////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints a message and aborts the thread
 *
 * @param[in] file  Source file of the caller
 * @param[in] func  Function name of the caller
 * @param[in] line  Line number of the caller
 * @param[in] msg   Message to print (printf style)
 *
 * Something irrecoverably bad happened. Print a panic message, then abort.
 *
 * Use the PANIC macros instead of calling this function directly.
 */
void user_panic_fn(const char *file, const char *func, int line, const char *msg, ...)
{
    va_list ap;
    char msg_str[128];
    va_start(ap, msg);
    vsnprintf(msg_str, sizeof(msg_str), msg, ap);
    va_end(ap);

    LocalState *local = get_local_state();
    const char *name  = local->my_name;
    FILE       *log   = local->log_file;

    char str[256];
    snprintf(str, sizeof(str), "\x1B[1;91m<%s>%s() %s:%d\n%s\x1B[0m\n", name, func, file, line, msg_str);
    fwrite(str, sizeof(char), sizeof(str), log);
    write(STDERR_FILENO, str, sizeof(str));

    abort();
}