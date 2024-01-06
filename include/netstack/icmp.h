#ifndef __VNET_ICMP_H__
#define __VNET_ICMP_H__

#include "ip.h"
#include <event/buffer.h>
#include <netutil/icmp.h>
#include <netutil/etharp.h>

#define  NDP_HASH_BUCKETS     128

typedef struct icmp_state {
    // The hash table for IPv6 multicast addresses
    alignas(ATOMIC_ISOLATION) 
        HashTable  hosts;
    alignas(ATOMIC_ISOLATION) 
        HashBucket buckets[NDP_HASH_BUCKETS];

    mac_addr         my_mac;
    struct ip_state *ip;

} ICMP;
 
__BEGIN_DECLS

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip, mac_addr my_mac
);

void icmp_destroy(
    ICMP* icmp
);

errval_t icmpv6_marshal(
    ICMP* icmp, ipv6_addr_t dst_ip, uint8_t type, uint8_t code, Buffer buf
);

errval_t icmp_marshal(
    ICMP* icmp, ip_addr_t dst_ip, uint8_t type, uint8_t code, ICMP_data field, Buffer buf
);

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, Buffer buf
);

// Can't assume the destionation IP is our IP, it could be a multicast address, like 'ff02::1:ffdc:6aa7
errval_t icmpv6_unmarshal(
    ICMP* icmp, ipv6_addr_t src_ip, ipv6_addr_t dst_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_ICMP_H__
