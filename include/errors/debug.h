#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <errors/log.h>
#include <errors/errno.h>

__BEGIN_DECLS

void debug_err(const char *file, const char *func, int line, errval_t err, const char *msg, ...);

void user_panic_fn(const char *file, const char *func, int line, const char *msg, ...)
    __attribute__((noreturn));

#ifdef NDEBUG
#define DEBUG_ERR(err, msg, ...) ((void)(0 && err))
#else
#define DEBUG_ERR(err, msg, ...) debug_err(__BASEFILE__, __func__, __LINE__, err, msg)
#endif

#define PUSH_ERR_PRINT(ERR, ERRNAME, fmt, ...) \
    if(err_is_fail(ERR)) {  \
        DEBUG_ERR(ERR, fmt, ##__VA_ARGS__); \
        return err_push(ERR, ERRNAME); \
    } 
#define RETURN_ERR_PRINT(ERR, fmt, ...) \
    if(err_is_fail(ERR)) {  \
        DEBUG_ERR(ERR, fmt, ##__VA_ARGS__); \
        return ERR; \
    } 

/**
 * @brief Prints out a message and then aborts the domain
 */
#define USER_PANIC_ERR(err, msg, ...)                                           \
    do {                                                                        \
        LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                   \
        debug_err(__BASEFILE__, __func__, __LINE__, err, msg, ##__VA_ARGS__);   \
        abort();                                                                \
    } while (0)

/**
 * @brief Prints out a message and then aborts the domain
 */
#define USER_PANIC(msg, ...)                                                  \
    do {                                                                      \
        LOG(MODULE_LOG, LOG_LEVEL_PANIC, msg, ##__VA_ARGS__);                 \
        user_panic_fn(__BASEFILE__, __func__, __LINE__, msg, ##__VA_ARGS__);  \
    } while (0)
__END_DECLS

#endif // __DEBUG_H__