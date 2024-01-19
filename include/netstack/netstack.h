#ifndef __NETSTACK_NETWORK_H__
#define __NETSTACK_NETWORK_H__

#include <device/device.h>
#include "ethernet.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "udp.h"
#include "tcp.h"

typedef struct net_stack {
    NetDevice  *device;
    Ethernet   *ether;
    ARP        *arp;
    IP         *ip;
    ICMP       *icmp;
    UDP        *udp;
    TCP        *tcp;
    ip_addr_t   my_ipv4;
    ipv6_addr_t my_ipv6;
    mac_addr    my_mac;
} NetStack;

typedef struct rpc rpc_t;

__BEGIN_DECLS

errval_t netstack_init(NetStack* net, NetDevice* device);
void netstack_destroy(NetStack* net);
errval_t network_udp_bind_port(rpc_t* rpc, ip_context_t ip_to_recv, udp_port_t port_to_bind, udp_server_callback callback, void* st);
errval_t network_udp_unbind_port(rpc_t* rpc, udp_port_t port_to_unbind);
errval_t network_udp_send_packet(rpc_t* rpc, ip_context_t dst_ip, udp_port_t dst_port, const void* addr, size_t size);

__END_DECLS

#endif // __NETSTACK_NETWORK_H__