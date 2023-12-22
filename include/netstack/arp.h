#ifndef __VNET_ARP__
#define __VNET_ARP__

#include <netutil/ip.h>
#include "ethernet.h"

typedef struct arp_state {
    struct ethernet_state* ether;
    ip_addr_t ip;
    // collections_hash_table* hosts;
} ARP;

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
);

errval_t arp_marshal(
    ARP* arp, ip_addr_t dst_ip, uint16_t type, void* data_start
);

errval_t arp_send(
    ARP* arp, uint16_t opration,
    ip_addr_t dst_ip, mac_addr dst_mac
);

void arp_register(
    ARP* arp, ip_addr_t ip, mac_addr mac 
);

errval_t arp_lookup_ip (
    ARP* arp, mac_addr mac, ip_addr_t* ip
);

errval_t arp_lookup_mac(
    ARP* arp, ip_addr_t ip, mac_addr* mac
);

errval_t arp_unmarshal(
    ARP* arp, void* data, size_t size
);

void arp_dump(ARP* arp, char** result);

#endif //__VNET_ARP__