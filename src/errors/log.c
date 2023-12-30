#include <errors/log.h>
#include <stdio.h>
#include <event/states.h>
#include <time.h>
#include <errno.h>      //strerror

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
        case LOG_LEVEL_FATAL:   return "FATAL";
        case LOG_LEVEL_PANIC:   return "PANIC";
        default:                return "UNKNOWN";
    }
}

const char *level_colors[] = {
    [LOG_LEVEL_VERBOSE] = "", 
    [LOG_LEVEL_INFO]    = "\x1B[1m", 
    [LOG_LEVEL_DEBUG]   = "\x1B[0;36m", 
    [LOG_LEVEL_NOTE]    = "\x1B[1;34m", // Blue
    [LOG_LEVEL_WARN]    = "\x1B[0;33m", // Yellow
    [LOG_LEVEL_ERROR]   = "\x1B[0;91m", // Red
    [LOG_LEVEL_FATAL]   = "\x1B[0;95m", // Magenta
};

// Utility function to convert log module enum to string
static const char* module_to_string(enum log_module module) {
    switch (module) {
        case MODULE_LOG:       return "LOG ";
        case MODULE_IPC:       return "IPC ";
        case MODULE_EVENT:     return "EVNT";
        case MODULE_TIMER:     return "TIMR";
        case MODULE_DEVICE:    return "DVIC";
        case MODULE_ETHER:     return "ETHR";
        case MODULE_ARP:       return "ARP ";
        case MODULE_IP:        return "IP  ";
        case MODULE_ICMP:      return "ICMP";
        case MODULE_UDP:       return "UDP ";
        case MODULE_TCP:       return "TCP ";
        default:        return "UNKNOWN_MODULE";
    }
}

bool ansi_output;

errval_t log_init(const char* log_file, enum log_level log_level, bool ansi, FILE** ret_file) {
    char str[128];
    
    FILE *log = fopen(log_file, "w");
    if (log == NULL) {
        const char *error_msg = strerror(errno);
        sprintf(str, "Error opening file: %s because %s, going to use the standard output", error_msg, log_file);
        write(STDERR_FILENO, str, strlen(str));
        return EVENT_LOGFILE_CREATE;
    } 
    sprintf(str, "Opened log file at %s, level set as %s\n", log_file, level_to_string(log_level));
    fwrite(str, sizeof(char), strlen(str), log);

    // Set full buffer mode, size as 65536
#ifdef NDEBUG
    setvbuf(log, NULL, _IOFBF, 65536);      // Full buffer, 64KB buffer size
#else
    setvbuf(log, NULL, _IONBF, 0);          // No buffer, directly write to the file
#endif

    // TODO: read it from global state ?
    for (int i = 0; i < LOG_MODULE_COUNT; i++) {
        if (log_matrix[i] < log_level) {
            log_matrix[i] = log_level;
        }
    }
    
    // Use ANSI color or not
    ansi_output = ansi;
    
    *ret_file = log;

    return SYS_ERR_OK;
}

void log_close(FILE* log) {
    LOG_ERR("Should flush every log file !");
    fflush(log);
    // Let the OS close the file to avoid race condition
    // fclose(log);
}

static int error_ansi(char* buf_after_leader, const size_t max_len, LocalState * local, const char* msg, va_list args) {
    const char *name  = local->my_name;

    time_t now; struct tm local_tm;
    time(&now); localtime_r(&now, &local_tm);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%b-%d %H:%M:%S", &local_tm);
    
    int len = 0;
    if (msg != NULL) {
        len = snprintf(buf_after_leader, max_len, "<%s>{%s}", name, time_str);
        len += vsnprintf(buf_after_leader + len, max_len - len, msg, args);
    }
    return len;
}

static int error_json(char* buf_after_leader, const size_t max_len, LocalState * local, const char* msg, va_list args) {
    
    const char *name  = local->my_name;
    
    time_t now; struct tm local_tm;
    time(&now); localtime_r(&now, &local_tm);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%b-%d %H:%M:%S", &local_tm);
    
    int len = 0;
    if (msg != NULL) {
        len = snprintf(buf_after_leader, max_len, "\"thread\": \"%s\", \"time\": \"%s\", \"message\": \"", name, time_str);
        len += vsnprintf(buf_after_leader + len, max_len - len, msg, args);
        len += snprintf(buf_after_leader + len, max_len - len, "\"");
    }

    return len;
}

void log_ansi(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...)
{
    char buffer[256]; 

    LocalState *local = get_local_state();
    FILE       *log   = local->log_file;

    const char *leader = level < LOG_LEVEL_PANIC ? level_colors[level] : "Unknown";
    
    int len_leader = snprintf(buffer, sizeof(buffer), "%s[%s-%s]", leader, level_to_string(level), module_to_string(module));
    int len_middle = 0;
    int len_suffix = 0;
    int len_tail   = 0;

    // Middle using error_assemble
    if (msg != NULL && len_leader < (int)sizeof(buffer)) {
        va_list ap;
        va_start(ap, msg);
        len_middle = error_ansi(buffer + len_leader, sizeof(buffer) - len_leader, local, msg, ap);    
        va_end(ap);
    }

    // Format the log suffix with file, line, and function
    len_suffix = snprintf(buffer + len_leader + len_middle, sizeof(buffer) - len_leader - len_middle,
                   " %s:%d->%s(): ", file, line, func);

    // Reset color at the end of the message
    const char *tail = level < LOG_LEVEL_INFO ? "\n" : "\x1B[0m\n";
    len_tail = snprintf(buffer + len_leader + len_middle + len_suffix, 
                        sizeof(buffer) - len_leader - len_middle - len_suffix,
                        tail);

    fwrite(buffer, sizeof(char), len_leader + len_middle + len_suffix + len_tail, log);
}

void log_json(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...)
{
    char buffer[512]; 

    LocalState *local = get_local_state();
    FILE       *log   = local->log_file;

    int len_prefix = snprintf(buffer, sizeof(buffer),
                              "{ \"level\": \"%s\", \"module\": \"%s\", \"file\": \"%s\", "
                              "\"line\": %d, \"function\": \"%s()\", ",
                              level_to_string(level), module_to_string(module), file, line, func);

    int len_message = 0;
    int len_tail   = 0;

    // Middle using error_assemble
    if (msg != NULL && len_prefix < (int)sizeof(buffer)) {
        va_list ap;
        va_start(ap, msg);
        len_message = error_json(buffer + len_prefix, sizeof(buffer) - len_message, local, msg, ap);    
        va_end(ap);
    }

    // Reset color at the end of the message
    const char *tail = "}\n";
    len_tail = snprintf(buffer + len_prefix + len_message,
                        sizeof(buffer) - len_prefix - len_message,
                        tail);

    fwrite(buffer, sizeof(char), len_prefix + len_message + len_tail, log);
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
void debug_err(const char *file, const char *func, int line, errval_t err, const char *msg, ...) {
    char buffer[512];
    LocalState *local = get_local_state();
    
    int len_leader = 0;
    int len_middle = 0;
    int len_tail   = 0;

    const char *leader = err_is_ok(err) ? "\x1B[1;32mSUCCESS" : "\x1B[1;91mERROR";
    len_leader = snprintf(buffer, sizeof(buffer), "%s", leader);

    if (msg != NULL && len_leader < (int)sizeof(buffer)) {
        va_list ap;
        va_start(ap, msg);
        len_middle = error_ansi(buffer + len_leader, sizeof(buffer) - len_leader, local, msg, ap);
        va_end(ap);
    }

    len_tail = snprintf(buffer + len_leader + len_middle, sizeof(buffer) - len_leader - len_middle, " %s:%d->%s() \x1B[0m\n", file, line, func);

    // 1. Directly write to stderr
    fwrite(buffer, sizeof(char), len_leader + len_middle + len_tail, stderr);
    if (err_is_fail(err)) {
        leader = "Error calltrace:\n";
        fwrite(leader, sizeof(char), strlen(leader), stderr);
        err_print_calltrace(err, stderr);
    }
    fflush(stderr);

    // 2. Flush all the log files
    fflush(local->log_file);
    fflush(g_states.log_file);
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
void user_panic_fn(const char *file, const char *func, int line, const char *msg, ...) {
    char buffer[128];
    LocalState *local = get_local_state();
    
    const char *leader = "\x1B[1;35mPANIC ";
    int len_leader = snprintf(buffer, sizeof(buffer), "%s", leader);

    va_list ap;
    va_start(ap, msg);
    int len_middle = error_ansi(buffer + len_leader, sizeof(buffer) - len_leader, local, msg, ap);
    va_end(ap);

    int len_suffix = snprintf(buffer + len_leader + len_middle, sizeof(buffer) - len_leader - len_middle,
                              " %s:%d->%s(): \x1B[0m\n", file, line, func);

    // Directly write to stderr
    write(STDERR_FILENO, buffer, len_leader + len_middle + len_suffix);
    
    // Flush all the log files
    fflush(local->log_file);
    fflush(g_states.log_file);

    abort();
}