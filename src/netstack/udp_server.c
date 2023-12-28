#include "udp_server.h"
#include <event/states.h>

errval_t udp_server_register(
    UDP* udp, rpc_t* rpc, const udp_port_t port, const udp_server_callback callback
) {
    assert(udp);
    errval_t err_get, err_insert;

    UDP_server *get_server = NULL; 
    UDP_server *new_server = calloc(1, sizeof(UDP_server));
    *new_server = (UDP_server) {
        .max_worker = g_states.max_workers_for_single_udp_server,
        .is_live    = true,
        .sema       = { { 0 } },
        .udp        = udp,
        .rpc        = rpc,
        .port       = port,
        .callback   = callback,
    };

    if (sem_init(&new_server->sema, 0, new_server->max_worker) != 0){
        UDP_ERR("Can't initialize the semaphore of UDP server");
        return SYS_ERR_INIT_FAIL;
    }

    //TODO: reconsider the multithread contention here
    err_get = hash_get_by_key(&udp->servers, UDP_HASH_KEY(port), (void**)&get_server);
    switch (err_no(err_get))
    {
    case SYS_ERR_OK:
    {    // We may meet a dead server, since the hash table is add-only, we can't remove it
        assert(get_server);
        sem_wait(&get_server->sema);

        if (get_server->is_live == false)   // Dead Server
        {
            for (size_t i = 0; i < get_server->max_worker - 1; i++) {
                sem_wait(&get_server->sema);
            }   // Wait until all workers' done with this server
            assert(get_server->udp  == udp);
            assert(get_server->port == port);
            assert(get_server->max_worker == 8);
            get_server->rpc      = rpc;
            get_server->callback = callback;
            get_server->is_live  = true;

            for (size_t i = 0; i < get_server->max_worker; i++) {
                sem_post(&get_server->sema);
            }   // Now the new server is ready
            assert(sem_destroy(&new_server->sema) == 0);
            free(new_server);
            return SYS_ERR_OK;
        }
        else    // Live server
        {
            sem_post(&get_server->sema);
            assert(sem_destroy(&new_server->sema) == 0);
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

    err_insert = hash_insert(&udp->servers, UDP_HASH_KEY(port), get_server, false);
    switch (err_no(err_insert)) 
    {
    case SYS_ERR_OK:
        return SYS_ERR_OK;
    case EVENT_HASH_EXIST_ON_INSERT:
        //TODO: consider if multiple threads try to register the port
        assert(sem_destroy(&new_server->sema) == 0);
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
        sem_wait(&server->sema);
        if (server->is_live == false) {
            sem_post(&server->sema);
            UDP_ERR("A process try to de-register a dead UDP server on this port: %d", port);
            return NET_ERR_UDP_PORT_NOT_REGISTERED;
        } else {
            server->is_live = false;
            sem_post(&server->sema);
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