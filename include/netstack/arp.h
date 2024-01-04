#ifndef __VNET_ARP__
#define __VNET_ARP__

#include <netutil/ip.h>
#include <pthread.h>
#include <lock_free/hash_table.h>
#include <event/buffer.h>

#define  ARP_HASH_BUCKETS     128

__BEGIN_DECLS

/// Forward Declaration
typedef struct ethernet_state Ethernet;

typedef struct arp_state {
    alignas(ATOMIC_ISOLATION) 
        HashTable  hosts;    // Must be 128-bytes aligned
    alignas(ATOMIC_ISOLATION) 
        HashBucket buckets[ARP_HASH_BUCKETS];
    Ethernet      *ether;
    ip_addr_t      ip;
} ARP __attribute__((aligned(ATOMIC_ISOLATION)));

typedef uint64_t ARP_Hash_key;
#define ARP_HASH_KEY(ip)   (void*)(ARP_Hash_key)(ip)
static_assert(sizeof(ARP_Hash_key) == sizeof(void*), "The size of Hash_key must be equal to the size of a pointer");
static_assert(sizeof(void*)        >= sizeof(mac_addr), "We use pointer as key(mac_addr), so the size of pointer must be larger than the size of mac_addr");

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
);

void arp_destroy(
    ARP* arp
);

#define ARP_HEADER_RESERVE     sizeof(struct eth_hdr)
errval_t arp_marshal(
    ARP* arp, uint16_t opration,
    ip_addr_t dst_ip, mac_addr dst_mac, Buffer buf
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
    ARP* arp, Buffer buf
);

void arp_dump(ARP* arp, char** result);

__END_DECLS

#endif //__VNET_ARP__
