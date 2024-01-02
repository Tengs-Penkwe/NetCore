#ifndef __NETCORE_ERRNO_H__
#define __NETCORE_ERRNO_H__

#include <stdint.h>
#include <stdbool.h>
#include <bitmacros.h>
#include <stdio.h>      // FILE

typedef uintptr_t errval_t;
// How many bits does a push or pop shifts
#define ERR_SHIFT   8
#define ERR_MASK    MASK_T(errval_t, 8)

// Define domains and error codes
#define OK_CODES \
    X(SYS_ERR_OK,                      "General OK") \
    X(COUNT_THROW,                     "Count for THROW codes, shouldn't happen") \
    X(NET_THROW_SUBMIT_EVENT,          "Successfully Submit the event, the buffer is re-used, free the memory later") \
    X(NET_THROW_TCP_ENQUEUE,           "Successfully Enqueued a TCP message, need to free memory later") \
    X(NET_THROW_IPv4_SEG,              "We need to assemble this IP message, free the memory later !") \
    X(COUNT_OK,                        "Count for OK codes, shouldn't happen") \

#define SYSTEM_ERR_CODES \
    X(SYS_ERR_WRONG_CONFIG,           "Unknow Configuration") \
    X(SYS_ERR_ALLOC_FAIL,             "Some kind of allocation (malloc, new) failed") \
    X(SYS_ERR_INIT_FAIL,              "Some kind of initializaton (thread, mutex) failed") \
    X(SYS_ERR_NOT_IMPLEMENTED,        "This function isn't implemented yet") \
    X(SYS_ERR_BAD_ALIGNMENT,          "The alignment requirement is not satisfied") \

#define EVENT_ERR_CODES \
    X(EVENT_ERR_THREAD_CREATE,        "Can't create the thread for event") \
    X(EVENT_ERR_SIGNAL_INIT,          "Can't initialize the signal") \
    X(EVENT_LOGFILE_CREATE,           "Can't create the log file") \
    X(EVENT_ENQUEUE_FULL,             "The task queue is full")  \
    X(EVENT_DEQUEUE_EMPTY,            "The task queue is empty")  \
    X(EVENT_MEMPOOL_EMPTY,            "The memory pool is empty")  \
    X(EVENT_HASH_EXIST_ON_INSERT,     "The hash table is configure as none-overwritten for duplicated key")  \
    X(EVENT_HASH_OVERWRITE_ON_INSERT, "Overwritten the value of duplicated key since it's configured in this way")  \
    X(EVENT_HASH_NOT_EXIST,           "The element associated with the key doesn't exist in the hash table")  \
    X(EVENT_LIST_EXIST_ON_INSERT,     "The add-only list is configure as none-overwritten for duplicated key")  \
    X(EVENT_LIST_NOT_EXIST,           "The element associated with the key doesn't exist in the add-only list")  \

#define NETWORK_ERR_CODES \
    X(NET_ERR_DEVICE_INIT,             "Can't initialize the network device") \
    X(NET_ERR_DEVICE_SEND,             "Can't send raw packet by network device") \
    X(NET_ERR_DEVICE_FAIL_POLL,        "Can't poll on the device !") \
    X(NET_ERR_DEVICE_GET_MAC,          "Can't get MAC address of my network device") \
    X(NET_ERR_ETHER_NULL_MAC,          "Destination MAC address of received message is a NULL MAC") \
    X(NET_ERR_ETHER_WRONG_MAC,         "Destination MAC address of received message doesn't meet with our MAC") \
    X(NET_ERR_ETHER_NO_MAC,            "Can't get MAC address of my ethernet") \
    X(NET_ERR_ETHER_NO_IP_ADDRESS,     "We don't have IP address associated with given MAC address") \
    X(NET_ERR_ETHER_UNKNOWN_TYPE,      "Unknown Ethernet type other than ARP or IP") \
    X(NET_ERR_ARP_WRONG_FIELD,         "Wrong Field in ARP packet") \
    X(NET_ERR_ARP_WRONG_IP_ADDRESS,    "Wrong Destination IP address for the ARP request") \
    X(NET_ERR_ARP_NO_MAC_ADDRESS,      "Can't find the MAC address of given IPv4 address") \
    X(NET_ERR_IP_VERSION,              "The IP version of received message is wrong") \
    X(NET_ERR_IPv4_WRONG_FIELD,        "Wrong Field in IPv4 packet") \
    X(NET_ERR_IPv4_WRONG_CHECKSUM,     "Wrong checksum in IPv4 packet") \
    X(NET_ERR_IPv4_WRONG_IP_ADDRESS,   "Wrong Destination IP address for the IPv4 packet") \
    X(NET_ERR_IPv4_WRONG_PROTOCOL,     "Wrong Protocol type in the IPv4 packet") \
    X(NET_ERR_IPv4_DUPLITCATE_SEG,     "We received a same IP packet segmentation twice") \
    X(NET_ERR_NDP_WRONG_FIELD,         "Wrong Field in NDP packet") \
    X(NET_ERR_NDP_NO_MAC_ADDRESS,      "Can't find the MAC address of given IPv6 address") \
    X(NET_ERR_IPv6_WRONG_FIELD,        "Wrong Field in IPv6 packet") \
    X(NET_ERR_IPv6_NEXT_HEADER,        "Not Supported next header in IPv6 packet") \
    X(NET_ERR_ICMP_WRONG_CHECKSUM,     "Wrong checksum in ICMP packet") \
    X(NET_ERR_ICMP_WRONG_TYPE,         "Wrong ICMP message type") \
    X(NET_ERR_UDP_WRONG_FIELD,         "Wrong Field in UDP packet") \
    X(NET_ERR_UDP_PORT_REGISTERED,     "This UDP Port has already been registered") \
    X(NET_ERR_UDP_PORT_NOT_REGISTERED, "This UDP Port isn't registered") \
    X(NET_ERR_TCP_WRONG_FIELD,         "Wrong Field in TCP header") \
    X(NET_ERR_TCP_PORT_REGISTERED,     "This TCP Port has already been registered") \
    X(NET_ERR_TCP_PORT_NOT_REGISTERED, "This TCP Port isn't registered") \
    X(NET_ERR_TCP_NO_CONNECTION,       "TCP received a message not SYN for a not established connection, or send to a place with no connection") \
    X(NET_ERR_TCP_BAD_STATE,           "TCP connection received a message with impossible state") \
    X(NET_ERR_TCP_WRONG_SEQUENCE,      "The Sequence number of this TCP message is wrong") \
    X(NET_ERR_TCP_WRONG_ACKNOWLEDGE,   "The Acknowledge number of this TCP message is wrong") \
    X(NET_ERR_TCP_MAX_CONNECTION,      "The TCP server is there, but it has too many connections") \
    X(NET_ERR_TCP_QUEUE_FULL,          "The Message queue of TCP is full") \
    X(NET_ERR_TCP_CREATE_WORKER,       "Can't Create worker thread for TCP server") \

#define IPC_ERR_CODES \
    X(IPC_ERR_INIT,                    "Can't initialize the IPC Module") \


enum err_code {
#define X(code, str) code,
    OK_CODES
    SYSTEM_ERR_CODES
    EVENT_ERR_CODES
    NETWORK_ERR_CODES
    IPC_ERR_CODES
#undef X
};

extern const char* err_code_strings[];

/* prototypes: */
 
__BEGIN_DECLS
char* err_getstring(errval_t errval);
const char* err_code_to_string(enum err_code code);
void err_print_calltrace(errval_t err, FILE* log);

static inline bool err_is_fail(errval_t errval);
static inline bool err_is_ok(errval_t errval);
static inline bool err_is_throw(errval_t errval);

static inline enum err_code err_no(errval_t errval);
static inline errval_t err_pop(errval_t errval);
static inline errval_t err_push(errval_t errval,enum err_code errcode);
 
/* function definitions: */
 
static inline bool err_is_fail(errval_t errval) {
    enum err_code code = err_no(errval);
    return (code > COUNT_OK);
}

static inline bool err_is_throw(errval_t errval)
{
    enum err_code code = err_no(errval);
    return (code > COUNT_THROW && code < COUNT_OK);
}

static inline bool err_is_ok(errval_t errval)
{
    enum err_code code = err_no(errval);
    return (code < COUNT_OK);
}

static inline enum err_code err_no(errval_t errval) 
{
    return (enum err_code)(errval & ERR_MASK);
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
