#ifndef __NDP_H__
#define __NDP_H__

#include <stdint.h>
#include "ip.h"

/* NDP message types */
#define NDP_ROUTER_SOLICITATION       133
#define NDP_ROUTER_ADVERTISEMENT      134
#define NDP_NEIGHBOR_SOLICITATION     135
#define NDP_NEIGHBOR_ADVERTISEMENT    136
#define NDP_REDIRECT                  137

/* NDP option types */
#define NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS     1
#define NDP_OPTION_TARGET_LINK_LAYER_ADDRESS     2
#define NDP_OPTION_PREFIX_INFORMATION            3
#define NDP_OPTION_REDIRECTED_HEADER             4
#define NDP_OPTION_MTU                           5

/* Common NDP header format */
struct ndp_hdr {
    uint8_t  type;       /* Message type */
    uint8_t  code;       /* Message code */
    uint16_t checksum;   /* Checksum */
} __attribute__((__packed__));

/* NDP Router Solicitation */
struct ndp_router_solicitation {
    uint32_t reserved;
    // Options follow...
};

/* NDP Router Advertisement */
struct ndp_router_advertisement {
    uint8_t  cur_hop_limit;
    uint8_t  flags;
    uint16_t router_lifetime;
    uint32_t reachable_time;
    uint32_t retrans_timer;
    // Options follow...
};

/* NDP Neighbor Solicitation */
struct ndp_neighbor_solicitation {
    uint32_t reserved;
    ipv6_addr_t to_addr;
    // Options follow...
};

/* NDP Neighbor Advertisement */
struct ndp_neighbor_advertisement {
    uint32_t    flags;
    ipv6_addr_t to_addr;
    // Options follow...
};

/* NDP Redirect */
struct ndp_redirect {
    uint32_t    reserved;
    ipv6_addr_t to_addr;
    ipv6_addr_t from_addr;
    // Options follow...
};

/* NDP Option format */
struct ndp_option {
    uint8_t type;   /* Type of the option */
    uint8_t length; /* Length of the option in units of 8 octets */
    // Option Data follows...
};

typedef struct ndp_data {
    union {
        struct ndp_router_solicitation    router_solicitation;
        struct ndp_router_advertisement   router_advertisement;
        struct ndp_neighbor_solicitation  neighbor_solicitation;
        struct ndp_neighbor_advertisement neighbor_advertisement;
        struct ndp_redirect               redirect;
    };
} NDP_data;

/* Function declarations */
// Functions to parse and process NDP messages would be defined here.

#endif // __NDP_H__
