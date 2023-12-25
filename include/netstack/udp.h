#ifndef __VNET_UDP_H__
#define __VNET_UDP_H__

#include <netutil/udp.h>

#define UPD_DEFAULT_BND     128

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
    struct aos_rpc     *chan;
    udp_port_t          port;
    udp_server_callback callback;
} UDP_server ;

typedef struct udp_state {
    struct ip_state* ip;
    // collections_hash_table* servers;
} UDP;

errval_t udp_init(
    UDP* udp, struct ip_state* ip
);

errval_t udp_marshal(
    UDP* udp, const ip_addr_t dst_ip, const udp_port_t src_port, const udp_port_t dst_port,
    void* addr, size_t size
);

errval_t udp_unmarshal(
    UDP* udp, const ip_addr_t src_ip, void* addr, size_t size
);

errval_t udp_server_register(
    UDP* udp, int fd, struct aos_rpc* rpc, const udp_port_t port, const udp_server_callback callback
);

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
);

#endif  //__VNET_UDP_H__
