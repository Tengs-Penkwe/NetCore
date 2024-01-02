#ifndef __NETSTACK_NDP_H
#define __NETSTACK_NDP_H

#include <event/buffer.h> 
#include <netutil/ndp.h>
#include <netutil/ip.h>

typedef struct ndp_state {
    struct ethernet_state *ether;
    ipv6_addr_t            my_ipv6;

} NDP ;

errval_t ndp_init(
    NDP* ndp, struct ethernet_state* ether, ipv6_addr_t my_ipv6
);

void ndp_destroy(
    NDP* ndp
);

errval_t ndp_lookup_mac(
    NDP* ndp, ipv6_addr_t dst_ip, mac_addr* ret_mac
);

errval_t ndp_unmarshal(
    NDP* ndp, Buffer buf
);

#endif  //__NETSTACK_NDP_H__
