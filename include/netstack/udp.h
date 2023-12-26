#ifndef __VNET_UDP_H__
#define __VNET_UDP_H__

#include <netutil/udp.h>
#include <lock_free/hash_table.h>

#define UDP_DEFAULT_BND     256

// Forward Declaration
struct udp_state;
struct udp_server;

typedef void (*udp_server_callback) (
    struct udp_server* server,
    const void* data, const size_t size,
    const ip_addr_t src_ip, const udp_port_t src_port
);

#define UDP_KEY(port)    (uint64_t)(port)

typedef struct udp_server {
    struct udp_state   *udp;
    int                 fd;
    udp_port_t          port;
    udp_server_callback callback;
} UDP_server ;

typedef struct udp_state {
    alignas(ATOMIC_ISOLATION) 
        HashTable    servers;
    alignas(ATOMIC_ISOLATION) 
        HashBucket   buckets[UDP_DEFAULT_BND];
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
    void* addr, size_t size
);

errval_t udp_unmarshal(
    UDP* udp, const ip_addr_t src_ip, void* addr, size_t size
);

errval_t udp_server_register(
    UDP* udp, int fd, const udp_port_t port, const udp_server_callback callback
);

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
);

__END_DECLS

#endif  //__VNET_UDP_H__
