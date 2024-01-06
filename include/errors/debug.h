#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <errors/log.h>
#include <errors/errno.h>

__BEGIN_DECLS

void debug_err(const char *file, const char *func, int line, errval_t err, const char *msg, ...) __attribute__((format(printf, 5, 0)));
void user_panic_fn(const char *file, const char *func, int line, const char *msg, ...) __attribute__((format(printf, 4, 0))) __attribute__((noreturn));

// #define NDEBUG

#ifdef NDEBUG
#undef assert
#undef _assert
#define assert(expr) ((void)(expr))
#define _assert(expr) ((void)(expr))
#define DEBUG_ERR(err, msg, ...) ((void)(0 && err))
#define USER_PANIC_ERR(err, msg, ...)                                       \
    LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                   \
    debug_err(__BASEFILE__, __func__, __LINE__, err, msg, ##__VA_ARGS__);   
#define USER_PANIC(msg, ...)                                                \
    LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                 

#else

#define DEBUG_ERR(err, msg, ...) debug_err(__BASEFILE__, __func__, __LINE__, err, msg)
#define USER_PANIC_ERR(err, msg, ...)                                           \
    do {                                                                        \
        LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                   \
        debug_err(__BASEFILE__, __func__, __LINE__, err, msg, ##__VA_ARGS__);   \
        abort();                                                                \
    } while (0)
#define USER_PANIC(msg, ...)                                                  \
    do {                                                                      \
        LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                 \
        user_panic_fn(__BASEFILE__, __func__, __LINE__, msg, ##__VA_ARGS__);  \
    } while (0)
#endif

#define DEBUG_FAIL_PUSH(ERR, ERRNAME, fmt, ...) \
    if(err_is_fail(ERR)) {                      \
        if (!err_is_throw(ERR))                 \
            DEBUG_ERR(ERR, fmt, ##__VA_ARGS__); \
        return err_push(ERR, ERRNAME);          \
    } 
#define DEBUG_FAIL_RETURN(ERR, fmt, ...)        \
    if(err_is_fail(ERR)) {                      \
        if (!err_is_throw(ERR))                 \
            DEBUG_ERR(ERR, fmt, ##__VA_ARGS__); \
        return ERR;                             \
    } 

__END_DECLS

#endif // __DEBUG_H__
