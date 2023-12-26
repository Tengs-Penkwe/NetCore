#ifndef __VNET_ARP__
#define __VNET_ARP__

#include <netutil/ip.h>
#include <pthread.h>
#include <lock_free/hash_table.h>

__BEGIN_DECLS

/// Forward Declaration
typedef struct ethernet_state Ethernet;

typedef struct arp_state {
    alignas(ATOMIC_ISOLATION) 
        HashTable hosts;    // Must be 128-bytes aligned
    Ethernet     *ether;
    ip_addr_t     ip;
} ARP;

#define ARP_HASH_KEY(ip)   (Hash_key)(ip)
static_assert(sizeof(Hash_key) == sizeof(void*));
static_assert(sizeof(void*)    >= sizeof(mac_addr));

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
);

void arp_destroy(
    ARP* arp
);

#define ARP_RESERVE_SIZE  sizeof(struct eth_hdr)
errval_t arp_send(
    ARP* arp, uint16_t opration,
    ip_addr_t dst_ip, mac_addr dst_mac
);

void arp_register(
    ARP* arp, ip_addr_t ip, mac_addr mac 
);

errval_t mac_lookup_and_send(
    ARP* arp, ip_addr_t dst_ip, mac_addr* dst_mac
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