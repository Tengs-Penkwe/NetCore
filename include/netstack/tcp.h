#ifndef __VNET_TCP_H__
#define __VNET_TCP_H__

#include <netutil/tcp.h>
#include <lock_free/hash_table.h>
#include <lock_free/bdqueue.h>
#include <stdatomic.h>

#define TCP_SERVER_BUCKETS    64
// The handling of TCP connection must be single-thread, so, for a message 
#define TCP_QUEUE_NUMBER      8
#define TCP_QUEUE_SIZE        64

__BEGIN_DECLS

typedef struct tcp_server TCP_server;
typedef struct rpc rpc_t;

typedef void (*tcp_server_callback) (
    struct tcp_server* server,
    const void* data, const size_t size,
    const ip_addr_t src_ip, const tcp_port_t src_port
);

typedef struct tcp_state {
    /// @brief The hash table and buckets of TCP servers
    alignas(ATOMIC_ISOLATION) 
        HashTable    servers;
    alignas(ATOMIC_ISOLATION) 
        HashBucket   buckets[TCP_SERVER_BUCKETS];

    /// @brief The Queue of TCP message, I have spinlock here since it must be single-threaded
    alignas(ATOMIC_ISOLATION)
        BdQueue      msg_queue[TCP_QUEUE_NUMBER];
    atomic_flag      que_locks[TCP_QUEUE_SIZE];
    size_t           queue_num;
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
    uint8_t* addr, size_t size
);

errval_t tcp_unmarshal(
    TCP* tcp, const ip_addr_t src_ip, uint8_t* addr, uint16_t size
);

__END_DECLS

#endif  //__VNET_TCP_H__