#include <netstack/network.h>
#include <netutil/ip.h>
#include <netutil/dump.h>

errval_t network_init(NetWork* net, NetDevice* device) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    
    // 1. Set up the ethernet
    Ethernet* ether = aligned_alloc(ATOMIC_ISOLATION, sizeof(Ethernet));
    assert(ether); memset(ether, 0x00, sizeof(Ethernet));
    err = ethernet_init(device, ether);
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't Initialize the Ethernet Module");

    *net = (NetWork) {
        .ether   = ether,
        .device  = ether->device,
        .arp     = ether->arp,
        .ip      = ether->ip,
        .icmp    = ether->ip->icmp,
        .udp     = ether->ip->udp,
        .tcp     = ether->ip->tcp,
        .my_ipv4 = ether->ip->my_ipv4,
        .my_ipv6 = ether->ip->my_ipv6,
        .my_mac  = ether->my_mac,
    };

    return err;
}

void network_destroy(NetWork* net) {
    assert(net);
    ethernet_destroy(net->ether);

    char ipv4_str[IPv4_ADDRESTRLEN];
    char ipv6_str[IPv6_ADDRESTRLEN];
    format_ipv4_addr(net->my_ipv4, ipv4_str, sizeof(ipv4_str));
    format_ipv6_addr(net->my_ipv6, ipv6_str, sizeof(ipv6_str));
    
    LOG_NOTE("Network Module destroyed, IPv4: %s, IPv6: %s, MAC: %0.6lX",
            ipv4_str, ipv6_str, frommac(net->my_mac));
    free(net);
}