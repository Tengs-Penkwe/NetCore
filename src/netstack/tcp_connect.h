#ifndef __TCP_CONNECT_H__
#define __TCP_CONNECT_H__
#include "tcp_connect.h"
#include <netutil/ip.h>
#include <netutil/tcp.h>

#include <stdint.h>

typedef enum sever_state {
    LISTEN = 1,
    SYN_SENT,
    SYN_RECVD,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    TIME_WAIT,
    LAST_ACK,
    CLOSED,
} TCP_st;

typedef enum flags {
    TCP_FLAG_NONE = 0,
    TCP_FLAG_SYN,
    TCP_FLAG_ACK,
    TCP_FLAG_PSH_ACK,
    TCP_FLAG_SYN_ACK,
    TCP_FLAG_FIN,
    TCP_FLAG_FIN_ACK,
    TCP_FLAG_RST,
    TCP_FLAG_RST_ACK,
    TCP_FLAG_OTHER  // To represent other combinations or individual flags not covered
} Flags;

typedef struct tcp_message {
    union {
        struct {
            ip_addr_t    dst_ip;
            trans_port_t dst_port;
        } send;
        struct {
            ip_addr_t    src_ip;
            trans_port_t src_port;
        } recv;
    };
    Flags        flags;
    uint32_t     seqno;
    uint32_t     ackno;
    void        *data;
    size_t       size;
} TCP_msg ;    

static inline Flags get_tcp_flags(uint8_t flags) {
    // Check for combinations of flags first
    if (tcp_flag_is_set(flags, TCP_SYN) && tcp_flag_is_set(flags, TCP_ACK)) {
        return TCP_FLAG_SYN_ACK;
    } else if (tcp_flag_is_set(flags, TCP_FIN) && tcp_flag_is_set(flags, TCP_ACK)) {
        return TCP_FLAG_FIN_ACK;
    } else if (tcp_flag_is_set(flags, TCP_RST) && tcp_flag_is_set(flags, TCP_ACK)) {
        return TCP_FLAG_RST_ACK;
    } else if (tcp_flag_is_set(flags, TCP_PSH) && tcp_flag_is_set(flags, TCP_ACK)) {
        return TCP_FLAG_PSH_ACK;
    } 
    // Check for individual flags
    else if (tcp_flag_is_set(flags, TCP_SYN)) {
        return TCP_FLAG_SYN;
    } else if (tcp_flag_is_set(flags, TCP_ACK)) {
        return TCP_FLAG_ACK;
    } else if (tcp_flag_is_set(flags, TCP_FIN)) {
        return TCP_FLAG_FIN;
    } else if (tcp_flag_is_set(flags, TCP_RST)) {
        return TCP_FLAG_RST;
    } else {
        return TCP_FLAG_OTHER;
    }
}

// Function to convert Flags enum to a string
static inline const char* flags_to_string(Flags flag) {
    switch (flag) {
    case TCP_FLAG_NONE:     return "NONE";
    case TCP_FLAG_SYN:      return "SYN";
    case TCP_FLAG_ACK:      return "ACK";
    case TCP_FLAG_PSH_ACK:  return "PSH-ACK";
    case TCP_FLAG_SYN_ACK:  return "SYN-ACK";
    case TCP_FLAG_FIN:      return "FIN";
    case TCP_FLAG_FIN_ACK:  return "FIN-ACK";
    case TCP_FLAG_RST:      return "RST";
    case TCP_FLAG_RST_ACK:  return "RST-ACK";
    case TCP_FLAG_OTHER:    return "OTHER";
    default: return "UNKNOWN";
    }
}

static inline uint8_t flags_compile(Flags flag) {
    switch (flag) {
    case TCP_FLAG_SYN:      return TCP_SYN;
    case TCP_FLAG_ACK:      return TCP_ACK;
    case TCP_FLAG_PSH_ACK:  return TCP_PSH | TCP_ACK;
    case TCP_FLAG_SYN_ACK:  return TCP_SYN | TCP_ACK;
    case TCP_FLAG_FIN:      return TCP_FIN;
    case TCP_FLAG_FIN_ACK:  return TCP_FIN | TCP_ACK;
    case TCP_FLAG_RST:      return TCP_RST;
    case TCP_FLAG_RST_ACK:  return TCP_RST | TCP_ACK;
    case TCP_FLAG_OTHER:
    case TCP_FLAG_NONE:
    default:
        assert(0);
        return 0;
    }
}

void dump_tcp_msg(const TCP_msg *msg);

typedef struct tcp_connection TCP_conn;
errval_t conn_send_msg(
    TCP_conn* conn, void* addr, size_t size
);

errval_t conn_handle_msg(
    TCP_conn* conn, TCP_msg* msg
);

#endif // __TCP_CONNECT_H__