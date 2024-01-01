#ifndef __VNET_IPv6_H__
#define __VNET_IPv6_H__

#include <event/buffer.h>  // Buffer
#include <netutil/ip.h>
#include "ndp.h"
#include "ethernet.h"
#include "udp.h"
#include "tcp.h"

typedef struct ipv6_state {
    ipv6_addr_t my_ip;
    Ethernet   *ether;
    NDP        *ndp;
    UDP        *udp;
    TCP        *tcp;
} IPv6;

__BEGIN_DECLS

errval_t ipv6_init(
    IPv6* ip, ipv6_addr_t my_ip, Ethernet* ether, NDP* ndp, UDP* udp, TCP* tcp
);

void ipv6_destroy(
    IPv6* ip
);

errval_t ipv6_marshal(
    IPv6* ip, ipv6_addr_t dst_ip, Buffer buf
);

errval_t ipv6_unmarshal(
    IPv6* ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_IPv6_H__
