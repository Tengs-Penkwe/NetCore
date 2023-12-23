#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <stdint.h>
#include <stdbool.h>
#include <bitmacros.h>

typedef uintptr_t errval_t;
// How many bits does a push or pop shifts
#define ERR_SHIFT   8
#define ERR_MASK    MASK_T(errval_t, 8)

// Define domains and error codes
#define SYSTEM_ERR_CODES \
    X(SYS_ERR_OK,                     "SYS_ERR_OK") \
    X(SYS_ERR_FAIL,                   "SYS_ERR_FAIL") \
    X(SYS_ERR_NOT_IMPLEMENTED,        "This function isn't implemented yet")

#define NETWORK_ERR_CODES \
    X(NET_ERR_DEVICE_INIT,            "Can't initialize the network device") \
    X(NET_ERR_DEVICE_SEND,            "Can't send raw packet by network device") \
    X(NET_ERR_DEVICE_GET_MAC,         "Can't get MAC address of my network device") \
    X(NET_ERR_ETHER_NO_MAC,           "Can't get MAC address of my ethernet") \
    X(NET_ERR_ETHER_UNKNOWN_TYPE,     "Unknown Ethernet type other than ARP or IP") \
    X(NET_ERR_ARP_WRONG_FIELD,        "Wrong Field in ARP packet") \
    X(NET_ERR_ARP_WRONG_IP_ADDRESS,   "Wrong Destination IP address for the ARP request") \
    X(NET_ERR_IPv4_NO_MAC_ADDRESS,    "We don't have the MAC address for given IPv4 address") \

enum err_code {
#define X(code, str) code,
    SYSTEM_ERR_CODES
    NETWORK_ERR_CODES
#undef X
};

extern const char* err_code_strings[];

/* prototypes: */
 
__BEGIN_DECLS
char* err_getstring(errval_t errval);
const char* err_code_to_string(enum err_code code);
static inline bool err_is_fail(errval_t errval);
static inline bool err_is_ok(errval_t errval);
static inline enum err_code err_no(errval_t errval);
static inline errval_t err_pop(errval_t errval);
void err_print_calltrace(errval_t errval);
static inline errval_t err_push(errval_t errval,enum err_code errcode);
 
/* function definitions: */
 
static inline bool err_is_fail(errval_t errval) {
    enum err_code code = err_no(errval);
    return ((code != SYS_ERR_OK));
}
 
static inline bool err_is_ok(errval_t errval)
{
    return !err_is_fail(errval);
}
 
static inline enum err_code err_no(errval_t errval) 
{
    return (((enum err_code) (errval & ((1 << ERR_SHIFT) - 1))));
}
 
static inline errval_t err_pop(errval_t errval) 
{
    return ((errval >> ERR_SHIFT));
}
 
static inline errval_t err_push(errval_t errval, enum err_code errcode)
{
    return (((errval << ERR_SHIFT) | ((errval_t) (ERR_MASK & errcode))));
}
__END_DECLS
 
#endif // __ERRNO_H__
