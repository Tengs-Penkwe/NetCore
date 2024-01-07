#include "udp_server.h"
#include <event/states.h>
#include <stdatomic.h>      // atomic_thread_fence
#include <unistd.h>         // For usleep

errval_t udp_server_register(
    UDP* udp, rpc_t* rpc, const udp_port_t port, const udp_server_callback callback
) {
    assert(udp);
    errval_t err_get, err_insert;

    UDP_server *get_server = NULL; 
    UDP_server *new_server = calloc(1, sizeof(UDP_server));
    *new_server = (UDP_server) {
        .is_live    = true,
        .udp        = udp,
        .rpc        = rpc,
        .port       = port,
        .callback   = callback,
    };

    //TODO: reconsider the multithread contention here
    err_get = hash_get_by_key(&udp->servers, UDP_HASH_KEY(port), (void**)&get_server);
    switch (err_no(err_get))
    {
    case SYS_ERR_OK:
    {    // We may meet a dead server, since the hash table is add-only, we can't remove it
        assert(get_server);
        if (get_server->is_live == false)   // Dead Server
        {
            *get_server = *new_server;
            free(new_server);
            get_server->is_live = true;
            return SYS_ERR_OK;
        }
        else    // Live server
        {
            free(new_server);
            return NET_ERR_UDP_PORT_REGISTERED;
        }
        assert(0);
    }
    case EVENT_HASH_NOT_EXIST:
        break;
    default: USER_PANIC_ERR(err_get, "Unknown Error Code");
    }

    assert(err_no(err_get) == EVENT_HASH_NOT_EXIST);

    err_insert = hash_insert(&udp->servers, UDP_HASH_KEY(port), get_server);
    switch (err_no(err_insert)) 
    {
    case SYS_ERR_OK:
        return SYS_ERR_OK;
    case EVENT_HASH_EXIST_ON_INSERT:
        //TODO: consider if multiple threads try to register the port
        free(new_server);
        UDP_ERR("Another process also wants to register the UDP port and he/she gets it")
        return NET_ERR_UDP_PORT_REGISTERED;
    default: USER_PANIC_ERR(err_insert, "Unknown Error Code");
    }
}

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
) {
    assert(udp);
    errval_t err;
    
    UDP_server* server = NULL;
    err = hash_get_by_key(&udp->servers, UDP_HASH_KEY(port), (void**)&server);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        if (server->is_live == false) {
            UDP_ERR("A process try to de-register a dead UDP server on this port: %d", port);
            return NET_ERR_UDP_PORT_NOT_REGISTERED;
        } else {
            server->is_live = false;
            atomic_thread_fence(memory_order_release); 
            usleep(10000); //FIXME: This is unsafe, we should use something like Hazard Pointer
            UDP_INFO("We inactivated a UDP server at port: %d", port);
            return SYS_ERR_OK;
        }
        assert(0);
    case EVENT_HASH_NOT_EXIST:
        UDP_ERR("A process try to de-register a not existing UDP server on this port: %d", port);
        return NET_ERR_UDP_PORT_NOT_REGISTERED;
    default: USER_PANIC_ERR(err, "Unknown Error Code");
    }
}