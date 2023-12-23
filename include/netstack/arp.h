#ifndef __VNET_ARP__
#define __VNET_ARP__

#include <netutil/ip.h>
#include "khash.h"
#include <pthread.h>

__BEGIN_DECLS

/// The hash table of IP-MAC
KHASH_MAP_INIT_INT(ip_mac, mac_addr) 
/// Forward Declaration
typedef struct ethernet_state Ethernet;

typedef struct arp_state {
    struct ethernet_state *ether;
    ip_addr_t              ip;
    pthread_mutex_t        mutex;
    khash_t(ip_mac)       *hosts;   
} ARP;

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
);

#define ARP_RESERVE_SIZE  sizeof(struct eth_hdr)
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
    ARP* arp, uint8_t* data, size_t size
);

void arp_dump(ARP* arp, char** result);

__END_DECLS

#endif //__VNET_ARP__