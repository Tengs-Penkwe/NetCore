#ifndef __NDP_H__
#define __NDP_H__

#include <stdint.h>
#include "ip.h"         // for ipv6_addr_t

/* NDP option types */
enum ndp_option_type {
    NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS     = 1,
    NDP_OPTION_TARGET_LINK_LAYER_ADDRESS     = 2,
    NDP_OPTION_PREFIX_INFORMATION            = 3,
    NDP_OPTION_REDIRECTED_HEADER             = 4,
    NDP_OPTION_MTU                           = 5,
};

/* NDP Router Solicitation */
struct ndp_router_solicitation {
    uint32_t reserved;
    // Options follow...
} __attribute__((__packed__));

static_assert(sizeof(struct ndp_router_solicitation) == 4, "Invalid size");

/* NDP Router Advertisement */
struct ndp_router_advertisement {
    uint8_t  cur_hop_limit;
    uint8_t  flags;
    uint16_t router_lifetime;
    uint32_t reachable_time;
    uint32_t retrans_timer;
    // Options follow...
} __attribute__((__packed__));

static_assert(sizeof(struct ndp_router_advertisement) == 12, "Invalid size");

/* NDP Neighbor Solicitation */
struct ndp_neighbor_solicitation {
    uint32_t    reserved;
    ipv6_addr_t to_addr;
    // Options follow...
} __attribute__((__packed__));

static_assert(sizeof(struct ndp_neighbor_solicitation) == 20, "Invalid size");

/* NDP Neighbor Advertisement */
struct ndp_neighbor_advertisement {
    // uint8_t router    : 1;
    // uint8_t solicited : 1;
    // uint8_t override  : 1;
    // uint8_t reserved  : 29;
    uint32_t flags_reserved;
    ipv6_addr_t to_addr;
    // Options follow...
} __attribute__((__packed__));
#define NDP_NSA_RSO(router, solicited, override) ((router << 31) | (solicited << 30) | (override << 29) | 0x00000000)

static_assert(sizeof(struct ndp_neighbor_advertisement) == 20, "Invalid size");

/* NDP Redirect */
struct ndp_redirect {
    uint32_t    reserved;
    ipv6_addr_t to_addr;
    ipv6_addr_t from_addr;
    // Options follow...
} __attribute__((__packed__));

static_assert(sizeof(struct ndp_redirect) == 36, "Invalid size");

/* NDP Option format */
struct ndp_option {
    uint8_t type;   /* Type of the option */
    uint8_t length; /* Length of the option in units of 8 octets */
    uint8_t data[]; /* Option data */
} __attribute__((__packed__));

static_assert(sizeof(struct ndp_option) == 2, "Invalid size");

__BEGIN_DECLS

__END_DECLS 

#endif // __NDP_H__
