#ifndef __LOG_H__
#define __LOG_H__

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>     //strrchr

__BEGIN_DECLS

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
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_MODULE_LEVELS \
    X(LOG,               CURRENT_LOG_LEVEL) \
    X(DRIVER,            CURRENT_LOG_LEVEL) \
    X(ETHER,             CURRENT_LOG_LEVEL) \
    X(ARP,               CURRENT_LOG_LEVEL) \
    X(IP,                CURRENT_LOG_LEVEL) \
    X(ICMP,              CURRENT_LOG_LEVEL) \
    X(UDP,               CURRENT_LOG_LEVEL) \
    X(TCP,               CURRENT_LOG_LEVEL)

enum log_module {
#define X(module, level) module,
    LOG_MODULE_LEVELS
#undef X
    LOG_MODULE_COUNT,
};

extern enum log_level log_matrix[LOG_MODULE_COUNT];

#define __BASEFILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

void log_printf(enum log_module module, enum log_level level, int line, const char* func, const char* file, const char *msg, ...);

#define LOG(module, level, fmt, ...) \
    if(level >= log_matrix[module]) { \
        log_printf(module, level, __LINE__, __func__, __BASEFILE__, fmt, ##__VA_ARGS__); \
    }

#define LOG_VERBOSE(fmt, ...)         LOG(LOG, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)            LOG(LOG, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)           LOG(LOG, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_NOTE(fmt, ...)            LOG(LOG, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)            LOG(LOG, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)             LOG(LOG, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for DRIVER module
#define DRIVER_VERBOSE(fmt, ...)      LOG(DRIVER, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define DRIVER_INFO(fmt, ...)         LOG(DRIVER, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DRIVER_DEBUG(fmt, ...)        LOG(DRIVER, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define DRIVER_NOTE(fmt, ...)         LOG(DRIVER, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define DRIVER_WARN(fmt, ...)         LOG(DRIVER, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define DRIVER_ERR(fmt, ...)          LOG(DRIVER, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ETHER module
#define ETHER_VERBOSE(fmt, ...)       LOG(ETHER, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ETHER_INFO(fmt, ...)          LOG(ETHER, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ETHER_DEBUG(fmt, ...)         LOG(ETHER, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ETHER_NOTE(fmt, ...)          LOG(ETHER, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ETHER_WARN(fmt, ...)          LOG(ETHER, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ETHER_ERR(fmt, ...)           LOG(ETHER, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ARP module
#define ARP_VERBOSE(fmt, ...)         LOG(ARP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ARP_INFO(fmt, ...)            LOG(ARP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ARP_DEBUG(fmt, ...)           LOG(ARP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ARP_NOTE(fmt, ...)            LOG(ARP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ARP_WARN(fmt, ...)            LOG(ARP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ARP_ERR(fmt, ...)             LOG(ARP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for IP module
#define IP_VERBOSE(fmt, ...)          LOG(IP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define IP_INFO(fmt, ...)             LOG(IP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define IP_DEBUG(fmt, ...)            LOG(IP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define IP_NOTE(fmt, ...)             LOG(IP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define IP_WARN(fmt, ...)             LOG(IP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define IP_ERR(fmt, ...)              LOG(IP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for ICMP module
#define ICMP_VERBOSE(fmt, ...)        LOG(ICMP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define ICMP_INFO(fmt, ...)           LOG(ICMP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define ICMP_DEBUG(fmt, ...)          LOG(ICMP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ICMP_NOTE(fmt, ...)           LOG(ICMP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define ICMP_WARN(fmt, ...)           LOG(ICMP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define ICMP_ERR(fmt, ...)            LOG(ICMP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for UDP module
#define UDP_VERBOSE(fmt, ...)         LOG(UDP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define UDP_INFO(fmt, ...)            LOG(UDP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define UDP_DEBUG(fmt, ...)           LOG(UDP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define UDP_NOTE(fmt, ...)            LOG(UDP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define UDP_WARN(fmt, ...)            LOG(UDP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define UDP_ERR(fmt, ...)             LOG(UDP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// Define logging macros for TCP module
#define TCP_VERBOSE(fmt, ...)         LOG(TCP, LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
#define TCP_INFO(fmt, ...)            LOG(TCP, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define TCP_DEBUG(fmt, ...)           LOG(TCP, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define TCP_NOTE(fmt, ...)            LOG(TCP, LOG_LEVEL_NOTE, fmt, ##__VA_ARGS__)
#define TCP_WARN(fmt, ...)            LOG(TCP, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define TCP_ERR(fmt, ...)             LOG(TCP, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

__END_DECLS

#endif  //__LOG_H__
