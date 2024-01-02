#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include <netstack/udp.h>

#define UDP_HASH_KEY(port)    (Hash_key)(port)

typedef struct udp_server {
    bool                is_live;  // Since the hash table it inside is addonly, we need to mark if it is live
    struct udp_state   *udp;
    struct rpc         *rpc;
    udp_port_t          port;
    udp_server_callback callback;
} UDP_server ;

errval_t udp_server_register(
    UDP* udp, struct rpc* rpc, const udp_port_t port, const udp_server_callback callback
);

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
);


#endif // __TCP_UDP_SERVER_H__