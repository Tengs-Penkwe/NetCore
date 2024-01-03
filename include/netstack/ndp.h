#ifndef __NETSTACK_NDP_H
#define __NETSTACK_NDP_H

#include <event/buffer.h> 
#include <netutil/icmpv6.h>
#include <netutil/ip.h>
#include <lock_free/hash_table.h>

typedef struct icmp_state ICMP;
void ndp_register(
    ICMP* icmp, ipv6_addr_t ip, mac_addr mac
);

errval_t ndp_lookup_mac(
    ICMP* ndp, ipv6_addr_t dst_ip, mac_addr* ret_mac
);

#endif  //__NETSTACK_NDP_H__
