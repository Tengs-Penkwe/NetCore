#include <netstack/network.h>

errval_t network_init(NetWork* net, NetDevice* device) {
    errval_t err;
    
    // 1. Set up the ethernet
    Ethernet* ether = calloc(1, sizeof(Ethernet));
    assert(ether);
    err = ethernet_init(device, ether);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize Network Module");
        return SYS_ERR_INIT_FAIL;
    }

    *net = (NetWork) {
        .ether  = ether,
        .device = ether->device,
        .arp    = ether->arp,
        .ip     = ether->ip,
        .icmp   = ether->ip->icmp,
        .udp    = ether->ip->udp,
        .tcp    = ether->ip->tcp,
        .my_ip  = ether->ip->my_ip,
        .my_mac = ether->my_mac,
    };
    
    return SYS_ERR_OK;
}

void network_destroy(NetWork* net) {
    assert(net);
    ethernet_destroy(net->ether);
    LOG_ERR("TODO: free ");
}