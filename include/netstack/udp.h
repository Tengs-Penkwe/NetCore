#ifndef __VNET_UDP_H__
#define __VNET_UDP_H__

#include <netutil/udp.h>
#include <lock_free/hash_table.h>
#include <ipc/rpc.h>
#include <lock/spinlock.h>

#define UDP_DEFAULT_SERVER     128

// Forward Declaration
struct udp_state;
struct udp_server;
typedef struct rpc rpc_t;

typedef void (*udp_server_callback) (
    struct udp_server* server,
    const uint8_t* data, const size_t size,
    const ip_addr_t src_ip, const udp_port_t src_port
);

#define UDP_HASH_KEY(port)    (Hash_key)(port)

typedef struct udp_server {
    atomic_flag         lock;
    bool                is_live;
    struct udp_state   *udp;
    struct rpc         *rpc;
    udp_port_t          port;
    udp_server_callback callback;
} UDP_server ;

typedef struct udp_state {
    alignas(ATOMIC_ISOLATION) 
        HashTable    servers;
    alignas(ATOMIC_ISOLATION) 
        HashBucket   buckets[UDP_DEFAULT_SERVER];
    struct ip_state *ip;
} UDP __attribute__((aligned(ATOMIC_ISOLATION)));

__BEGIN_DECLS

errval_t udp_init(
    UDP* udp, struct ip_state* ip
);

void udp_destroy(
    UDP* udp
);

errval_t udp_marshal(
    UDP* udp, const ip_addr_t dst_ip, const udp_port_t src_port, const udp_port_t dst_port,
    uint8_t* addr, uint16_t size
);

errval_t udp_unmarshal(
    UDP* udp, const ip_addr_t src_ip, uint8_t* addr, uint16_t size
);

errval_t udp_server_register(
    UDP* udp, struct rpc* rpc, const udp_port_t port, const udp_server_callback callback
);

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
);

__END_DECLS

#endif  //__VNET_UDP_H__
