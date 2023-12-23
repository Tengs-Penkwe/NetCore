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
#include <debug.h>

#include <stdarg.h>         //va_list, va_start, va_end
#include <stdio.h>          //snprintf

#include <sys/syscall.h>   //syscall
#include <sys/types.h>     //pid_t

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

    pid_t tid = syscall(SYS_gettid);
    len = snprintf(str, sizeof(str), "%s<%lu>:%s() %s:%d\n%s: ", leader, tid, func, file, line, leader);
    if (msg != NULL) {
        va_list ap;
        va_start(ap, msg);
        len += vsnprintf(str + len, sizeof(str) - len, msg, ap);
        va_end(ap);
    }

    len += snprintf(str + len, sizeof(str) - len, "\x1B[0m\n");
    sys_print(str, len);

    if (err != 0) {
        leader = "Error calltrace:\n";
        sys_print(leader, strlen(leader));
        err_print_calltrace(err);
        sys_print("\n", 1);
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

    pid_t tid = syscall(SYS_gettid);
    char str[256];
    snprintf(str, sizeof(str), "\x1B[1;91m<%lu>%s() %s:%d\n%s\x1B[0m\n", tid, func, file, line, msg_str);
    sys_print(str, sizeof(str));

    abort();
}