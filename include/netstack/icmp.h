#ifndef __VNET_ICMP_H__
#define __VNET_ICMP_H__

#include "ip.h"

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
    uint8_t* data;
    uint16_t size;
} __attribute__((__packed__)) ICMP_data;

// STATIC_ASSERT_SIZEOF(union (icmp_data) echo, 4);

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip
);

errval_t icmp_send(
    ICMP* icmp, ip_addr_t dst_ip, uint8_t type, uint8_t code, ICMP_data field
);

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, uint8_t* addr, uint16_t size
);


#endif  //__VNET_ICMP_H__
