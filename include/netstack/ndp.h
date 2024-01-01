#ifndef __NETSTACK_NDP_H
#define __NETSTACK_NDP_H

#include <event/buffer.h> 
#include <netutil/ndp.h>
#include <netutil/ip.h>

typedef struct ethernet_state Ethernet;
typedef struct ipv6_state IPv6;
typedef struct ndp_state {
    ipv6_addr_t my_ip;
    Ethernet   *ether;
    IPv6       *ip6;
    
} NDP ;

errval_t ndp_init(
    NDP* ndp, ipv6_addr_t my_ip, IPv6* ip
);

void ndp_destroy(
    NDP* ndp
);

errval_t ndp_unmarshal(
    NDP* ndp, Buffer buf
);

#endif  //__NETSTACK_NDP_H__
