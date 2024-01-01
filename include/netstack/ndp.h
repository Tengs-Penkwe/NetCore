#ifndef __NETSTACK_NDP_H
#define __NETSTACK_NDP_H

#include <netutil/ndp.h>
#include <netutil/ip.h>

typedef struct ndp_state {
    ipv6_addr_t my_ip;

} NDP ;

errval_t ndp_init(
    NDP* ndp, ipv6_addr_t my_ip
);

void ndp_destroy(
    NDP* ndp
);

#endif  //__NETSTACK_NDP_H__


