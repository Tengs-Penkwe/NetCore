#ifndef __VNET_UDP_H__
#define __VNET_UDP_H__

#include <event/buffer.h>
#include <netutil/udp.h>
#include <lock_free/hash_table.h>
#include <interface/rpc.h>

#define UDP_DEFAULT_SERVER      64
#define UDP_MAX_PAYLOAD_SIZE    UINT16_MAX

// Forward Declaration
struct udp_state;
struct udp_server;
typedef struct rpc rpc_t;
typedef struct udp_server UDP_server;

typedef void (*udp_server_callback) (
    struct udp_server* server,
    Buffer buf,
    const ip_context_t src_ip, const udp_port_t src_port
);

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

errval_t udp_server_register(
    UDP* udp, struct rpc* rpc, const udp_port_t port, const udp_server_callback callback
);

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
);

errval_t udp_marshal(
    UDP* udp, const ip_context_t dst_ip, const udp_port_t src_port, const udp_port_t dst_port, Buffer buf
);

errval_t udp_unmarshal(
    UDP* udp, const ip_context_t src_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_UDP_H__
