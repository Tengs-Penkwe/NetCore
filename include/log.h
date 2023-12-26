#ifndef __LOG_H__
#define __LOG_H__

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>     //strrchr

__BEGIN_DECLS

// The file descriptor 1 refers to standard output (STDOUT)
#include <sys/unistd.h>
#define sys_print(buf, len) write(1, buf, len)

// X-macro lists for log levels and modules
#define LOG_LEVELS \
    X(VERBOSE, LOG_LEVEL_VERBOSE) \
    X(INFO,    LOG_LEVEL_INFO)    \
    X(DEBUG,   LOG_LEVEL_DEBUG)   \
    X(NOTE,    LOG_LEVEL_NOTE)    \
    X(WARN,    LOG_LEVEL_WARN)    \
    X(ERR,     LOG_LEVEL_ERROR)   \
    X(NONE,    LOG_LEVEL_NONE) 

//Allows developers to specify a log level for each message and configure the system 
// to only output messages at or above a certain level.
enum log_level {
#define X(level, def) def,
    LOG_LEVELS
#undef  X
};

// Current Log Level Setting
#define CURRENT_LOG_LEVEL LOG_LEVEL_VERBOSE

#define LOG_MODULE_LEVELS \
    X(LOG,                      CURRENT_LOG_LEVEL) \
    X(MODULE_EVENT,             CURRENT_LOG_LEVEL) \
    X(MODULE_TIMER,             CURRENT_LOG_LEVEL) \
    X(MODULE_DEVICE,            CURRENT_LOG_LEVEL) \
    X(MODULE_ETHER,             CURRENT_LOG_LEVEL) \
    X(MODULE_ARP,               CURRENT_LOG_LEVEL) \
    X(MODULE_IP,                CURRENT_LOG_LEVEL) \
    X(MODULE_ICMP,              CURRENT_LOG_LEVEL) \
    X(MODULE_UDP,               CURRENT_LOG_LEVEL) \
    X(MODULE_TCP,               CURRENT_LOG_LEVEL)

enum log_module {
#define X(module, level) module,
    LOG_MODULE_LEVELS
#undef X
    LOG_MODULE_COUNT,
};

#define __BASEFILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

void log_printf(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...);

// Defined in log.c to prevent duplicate definition
extern enum log_level log_matrix[LOG_MODULE_COUNT];

#define LOG(module, level, fmt, ...) \
    if(level >= log_matrix[module]) { \
        log_printf(module, level, __LINE__, __func__, __BASEFILE__, fmt, ##__VA_ARGS__); \
    }

// Define logging macros for general module
#define LOG_VERBOSE(fmt, ...)         LOG(LOG, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)            LOG(LOG, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)           LOG(LOG, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_NOTE(fmt, ...)            LOG(LOG, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)            LOG(LOG, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)             LOG(LOG, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for EVENT module
#define EVENT_VERBOSE(fmt, ...)       LOG(MODULE_EVENT, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define EVENT_INFO(fmt, ...)          LOG(MODULE_EVENT, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define EVENT_DEBUG(fmt, ...)         LOG(MODULE_EVENT, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define EVENT_NOTE(fmt, ...)          LOG(MODULE_EVENT, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define EVENT_WARN(fmt, ...)          LOG(MODULE_EVENT, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define EVENT_ERR(fmt, ...)           LOG(MODULE_EVENT, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for DRIVER module
#define TIMER_VERBOSE(fmt, ...)       LOG(MODULE_TIMER, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define TIMER_INFO(fmt, ...)          LOG(MODULE_TIMER, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define TIMER_DEBUG(fmt, ...)         LOG(MODULE_TIMER, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define TIMER_NOTE(fmt, ...)          LOG(MODULE_TIMER, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define TIMER_WARN(fmt, ...)          LOG(MODULE_TIMER, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define TIMER_ERR(fmt, ...)           LOG(MODULE_TIMER, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for DRIVER module
#define DEVICE_VERBOSE(fmt, ...)      LOG(MODULE_DEVICE, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define DEVICE_INFO(fmt, ...)         LOG(MODULE_DEVICE, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DEVICE_DEBUG(fmt, ...)        LOG(MODULE_DEVICE, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define DEVICE_NOTE(fmt, ...)         LOG(MODULE_DEVICE, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define DEVICE_WARN(fmt, ...)         LOG(MODULE_DEVICE, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define DEVICE_ERR(fmt, ...)          LOG(MODULE_DEVICE, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ETHER module
#define ETHER_VERBOSE(fmt, ...)       LOG(MODULE_ETHER, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ETHER_INFO(fmt, ...)          LOG(MODULE_ETHER, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ETHER_DEBUG(fmt, ...)         LOG(MODULE_ETHER, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ETHER_NOTE(fmt, ...)          LOG(MODULE_ETHER, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ETHER_WARN(fmt, ...)          LOG(MODULE_ETHER, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ETHER_ERR(fmt, ...)           LOG(MODULE_ETHER, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ARP module
#define ARP_VERBOSE(fmt, ...)         LOG(MODULE_ARP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ARP_INFO(fmt, ...)            LOG(MODULE_ARP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ARP_DEBUG(fmt, ...)           LOG(MODULE_ARP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ARP_NOTE(fmt, ...)            LOG(MODULE_ARP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ARP_WARN(fmt, ...)            LOG(MODULE_ARP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ARP_ERR(fmt, ...)             LOG(MODULE_ARP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for IP module
#define IP_VERBOSE(fmt, ...)          LOG(MODULE_IP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define IP_INFO(fmt, ...)             LOG(MODULE_IP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define IP_DEBUG(fmt, ...)            LOG(MODULE_IP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define IP_NOTE(fmt, ...)             LOG(MODULE_IP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define IP_WARN(fmt, ...)             LOG(MODULE_IP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define IP_ERR(fmt, ...)              LOG(MODULE_IP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ICMP module
#define ICMP_VERBOSE(fmt, ...)        LOG(MODULE_ICMP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ICMP_INFO(fmt, ...)           LOG(MODULE_ICMP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ICMP_DEBUG(fmt, ...)          LOG(MODULE_ICMP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ICMP_NOTE(fmt, ...)           LOG(MODULE_ICMP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ICMP_WARN(fmt, ...)           LOG(MODULE_ICMP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ICMP_ERR(fmt, ...)            LOG(MODULE_ICMP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for UDP module
#define UDP_VERBOSE(fmt, ...)         LOG(MODULE_UDP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define UDP_INFO(fmt, ...)            LOG(MODULE_UDP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define UDP_DEBUG(fmt, ...)           LOG(MODULE_UDP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define UDP_NOTE(fmt, ...)            LOG(MODULE_UDP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define UDP_WARN(fmt, ...)            LOG(MODULE_UDP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define UDP_ERR(fmt, ...)             LOG(MODULE_UDP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for TCP module
#define TCP_VERBOSE(fmt, ...)         LOG(MODULE_TCP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define TCP_INFO(fmt, ...)            LOG(MODULE_TCP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define TCP_DEBUG(fmt, ...)           LOG(MODULE_TCP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define TCP_NOTE(fmt, ...)            LOG(MODULE_TCP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define TCP_WARN(fmt, ...)            LOG(MODULE_TCP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define TCP_ERR(fmt, ...)             LOG(MODULE_TCP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

__END_DECLS

#endif  //__LOG_H__
