#ifndef __VNET_TCP_H__
#define __VNET_TCP_H__

#include <netutil/tcp.h>
#include "tcp_server.h"

#define TCP_DEFAULT_SERVER    128
#define TCP_DEFAULT_CONN      64

typedef struct tcp_state {
    struct ip_state* ip;
    // collections_hash_table *servers;
} TCP;

/// The key of a server inside the hash table 
#define TCP_KEY(port)   (uint64_t)(port)

errval_t tcp_init(
    TCP* tcp, struct ip_state* ip
);

errval_t tcp_marshal(
    TCP* tcp, const ip_addr_t dst_ip, const trans_port_t src_port, const trans_port_t dst_port,
    uint32_t seqno, uint32_t ackno, uint32_t window, uint16_t urg_prt, uint8_t flags,
    void* addr, size_t size
);

errval_t tcp_unmarshal(
    TCP* tcp, const ip_addr_t src_ip, void* addr, size_t size
);

errval_t tcp_server_register(
    TCP* tcp, int fd, struct aos_rpc* rpc, const trans_port_t port, const tcp_server_callback callback
);

errval_t tcp_server_deregister(
    TCP* tcp, const trans_port_t port
);

#endif  //__VNET_TCP_H__