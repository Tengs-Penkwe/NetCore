#ifndef __VNET_TCP_H__
#define __VNET_TCP_H__

#include <netutil/tcp.h>
#include <lock_free/hash_table.h>
#include <event/buffer.h>

#define TCP_SERVER_BUCKETS    64

__BEGIN_DECLS

typedef struct tcp_server TCP_server;

typedef void (*tcp_server_callback) (
    struct tcp_server* server,
    Buffer buf,
    const ip_context_t src_ip, const tcp_port_t src_port
);

/// The key of a server inside the hash table 
#define TCP_HASH_KEY(port)   (Hash_key)(port)

typedef struct tcp_state {
    /// @brief The hash table and buckets of TCP servers
    alignas(ATOMIC_ISOLATION) 
        HashTable    servers;
    alignas(ATOMIC_ISOLATION) 
        HashBucket   buckets[TCP_SERVER_BUCKETS];

    struct ip_state *ip;
} TCP __attribute__((aligned(ATOMIC_ISOLATION)));

errval_t tcp_init(
    TCP* tcp, struct ip_state* ip
);

void tcp_destroy(
    TCP* tcp
);

errval_t tcp_send(
    TCP* tcp, const ip_context_t dst_ip, const tcp_port_t src_port, const tcp_port_t dst_port,
    uint32_t seqno, uint32_t ackno, uint32_t window, uint16_t urg_prt, uint8_t flags,
    Buffer buf
);

errval_t tcp_unmarshal(
    TCP* tcp, const ip_context_t src_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_TCP_H__