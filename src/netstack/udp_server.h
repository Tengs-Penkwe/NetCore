#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include <netstack/udp.h>

#define UDP_HASH_KEY(port)    (void*)(Hash_key)(port)

typedef struct udp_server {
    bool                is_live;  // Since the hash table it inside is addonly, we need to mark if it is live
    struct udp_state   *udp;
    struct rpc         *rpc;
    udp_port_t          port;
    udp_server_callback callback;
} UDP_server ;

#endif // __TCP_UDP_SERVER_H__