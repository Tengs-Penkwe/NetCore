#include <netstack/tcp.h>
#include <netstack/ip.h>

errval_t tcp_init(TCP* tcp, IP* ip) {
    errval_t err;
    assert(tcp && ip);
    tcp->ip = ip;

    err = hash_init(&tcp->servers, tcp->buckets, TCP_DEFAULT_SERVER, HS_OVERWRITE_ON_EXIST);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of tcp servers");

    return SYS_ERR_OK;
}