#ifndef __NETSTACK_NETWORK_H__
#define __NETSTACK_NETWORK_H__

#include <device/device.h>
#include "ethernet.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "udp.h"
#include "tcp.h"

typedef struct net_work {
    NetDevice *device;
    Ethernet  *ether;
    ARP       *arp;
    IP        *ip;
    ICMP      *icmp;
    UDP       *udp;
    TCP       *tcp;
    ip_addr_t  my_ip;
    mac_addr   my_mac;
} NetWork;

__BEGIN_DECLS

errval_t network_init(NetWork* net, NetDevice* device);
void network_destroy(NetWork* net);

/// The implementations are across drivers
errval_t network_create_socket(int domain, int type, int protocal, int* fd);

/*
 * ===============================================================================================
 * TCP/UDP related operations
 * ===============================================================================================
 */
errval_t network_send_packet(struct rpc* rpc, ip_addr_t dst_ip, udp_port_t dst_port, const void* addr, size_t size);

errval_t network_bind_port(struct rpc* rpc, ip_addr_t ip_to_recv, udp_port_t port_to_bind);

errval_t network_set_listen(int fd, int max_conn);

errval_t network_unbind_port(int fd, udp_port_t port_to_bind);

errval_t network_back_message(ip_addr_t src_ip, udp_port_t src_port, void* data, size_t size);

errval_t network_store_message(int fd, ip_addr_t src_ip, udp_port_t src_port, void* data, size_t size);

errval_t network_get_message(int fd, ip_addr_t* src_ip, udp_port_t* src_port, char** data, ssize_t *ret_size);

/*
 * ===============================================================================================
 * ARP related operations
 * ===============================================================================================
 */
errval_t network_arp_dump(void);

/*
 * ===============================================================================================
 * ICMP related operations
 * ===============================================================================================
 */
errval_t network_send_icmp_packet(void);

__END_DECLS

#endif // __NETSTACK_NETWORK_H__