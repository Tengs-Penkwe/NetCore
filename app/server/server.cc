#include <netstack/netstack.h>
#include <sys/prctl.h>
#include <netutil/udp.h>    // IP_CONTEXT_ANY
#include "server.hh"
#include <stdlib.h>

void rpc_handler(void* args)
{
    (void)args;
}

void* localserver_thread(void* arg)
{
    (void)arg;
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    // 1. Set the thread name
    if (prctl(PR_SET_NAME, "localserver", 0, 0, 0) != 0) {
        TIMER_FATAL("Can't set the thread name");
        return NULL;
    }

    rpc_id_t id = {
        .name = "localserver",
    };

    // Set the RPC with the netstack driver
    rpc_t * rpc = calloc(1, sizeof(rpc_t)); assert(rpc);
    err = establish_rpc(rpc, RPC_LOCL, id, rpc_handler); 
    assert(err_is_ok(err)); 

    // err = network_udp_bind_port(rpc, IP_CONTEXT_ANY, 1234, NULL, NULL);
    // assert(err_is_ok(err));

    return NULL;
}