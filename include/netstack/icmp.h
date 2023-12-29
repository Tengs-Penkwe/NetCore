#ifndef __VNET_ICMP_H__
#define __VNET_ICMP_H__

#include "ip.h"
#include <netstack/type.h>

typedef struct icmp_state {
    struct ip_state* ip;

} ICMP;

struct icmp_echo {
    uint16_t id;
    uint16_t seqno;
} __attribute__((__packed__));

typedef struct icmp_data {
    union {
        struct icmp_echo echo;
    };
} __attribute__((__packed__)) ICMP_data;

// STATIC_ASSERT_SIZEOF(union (icmp_data) echo, 4);

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
