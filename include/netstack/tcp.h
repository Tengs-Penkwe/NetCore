#ifndef __VNET_TCP_H__
#define __VNET_TCP_H__

#include <netutil/tcp.h>
#include <lock_free/hash_table.h>

#define TCP_DEFAULT_SERVER    128

__BEGIN_DECLS

typedef struct tcp_server TCP_server;
typedef struct rpc rpc_t;

typedef void (*tcp_server_callback) (
    struct tcp_server* server,
    const void* data, const size_t size,
    const ip_addr_t src_ip, const tcp_port_t src_port
);

typedef struct tcp_state {
    alignas(ATOMIC_ISOLATION) 
        HashTable    servers;
    alignas(ATOMIC_ISOLATION) 
        HashBucket   buckets[TCP_DEFAULT_SERVER];
    struct ip_state *ip;
} TCP __attribute__((aligned(ATOMIC_ISOLATION)));

/// The key of a server inside the hash table 
#define TCP_HASH_KEY(port)   (Hash_key)(port)

errval_t tcp_init(
    TCP* tcp, struct ip_state* ip
);

errval_t tcp_marshal(
    TCP* tcp, const ip_addr_t dst_ip, const tcp_port_t src_port, const tcp_port_t dst_port,
    uint32_t seqno, uint32_t ackno, uint32_t window, uint16_t urg_prt, uint8_t flags,
    void* addr, uint16_t size
);

errval_t tcp_unmarshal(
    TCP* tcp, const ip_addr_t src_ip, void* addr, uint16_t size
);

errval_t tcp_server_register(
    TCP* tcp, struct rpc* rpc, const tcp_port_t port, const tcp_server_callback callback
);

errval_t tcp_server_deregister(
    TCP* tcp, const tcp_port_t port
);

__END_DECLS

#endif  //__VNET_TCP_H__