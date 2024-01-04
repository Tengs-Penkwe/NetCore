#ifndef __NETSTACK_NDP_H
#define __NETSTACK_NDP_H

#include <event/buffer.h> 
#include <netutil/icmpv6.h>
#include <netutil/ip.h>
#include <lock_free/hash_table.h>

// Ethernet: 18, IPv6: 40, ICMPv6: 8
#define NDP_HEADER_RESERVE   ROUND_UP(18 + 40 + 8, 8)

typedef struct icmp_state ICMP;

__BEGIN_DECLS

static inline int ipv6_key_cmp(void const *new_key, void const *existing_key) {
    ipv6_addr_t new        = *(ipv6_addr_t *)new_key;
    ipv6_addr_t exist      = *(ipv6_addr_t *)existing_key;
    return (new > exist) - (new < exist);
}

static inline void ipv6_key_hash(void const *key, lfds711_pal_uint_t *hash)
{
    *hash = 0;
    ipv6_addr_t key_128 = *(ipv6_addr_t *)key;
    LFDS711_HASH_A_HASH_FUNCTION(&key_128, sizeof(ipv6_addr_t), *hash)
    return;
}

void ndp_register(
    ICMP* icmp, ipv6_addr_t ip, mac_addr mac
);

errval_t ndp_marshal(
    ICMP* icmp, ipv6_addr_t dst_ip, uint8_t type, uint8_t code, Buffer buf
);

errval_t ndp_lookup_mac(
    ICMP* icmp, ipv6_addr_t dst_ip, mac_addr* ret_mac
);

errval_t ndp_neighbor_solicitation(
    ICMP* icmp, ipv6_addr_t dst_ip, uint8_t code, Buffer buf
);

__END_DECLS

#endif  //__NETSTACK_NDP_H__
