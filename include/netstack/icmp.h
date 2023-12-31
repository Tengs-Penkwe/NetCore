#ifndef __VNET_ICMP_H__
#define __VNET_ICMP_H__

#include "ip.h"
#include <event/buffer.h>
#include <netutil/icmp.h>

typedef struct icmp_state {
    struct ip_state* ip;

} ICMP;

typedef struct icmp_data {
    union {
        struct icmp_echo echo;
        struct icmp_qench qench;
        struct icmp_redirect redirect;
        struct icmp_timeex timeex;
        struct icmp_timestamp timestamp;
    };
} ICMP_data;
 
__BEGIN_DECLS

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip
);

errval_t icmp_marshal(
    ICMP* icmp, ip_addr_t dst_ip, uint8_t type, uint8_t code, ICMP_data field, Buffer buf
);

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_ICMP_H__
