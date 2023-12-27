#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include <semaphore.h>
#include <netstack/udp.h>

#define UDP_HASH_KEY(port)    (Hash_key)(port)

typedef struct udp_server {
    size_t              max_worker;
    bool                is_live;
    sem_t               sema;
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