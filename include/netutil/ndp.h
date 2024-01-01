#ifndef __NDP_H__
#define __NDP_H__

#include <stdint.h>

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
struct ndp_header {
    uint8_t  type;       /* Message type */
    uint8_t  code;       /* Message code */
    uint16_t checksum;   /* Checksum */
} __attribute__((__packed__));

/* NDP Router Solicitation */
struct ndp_router_solicitation {
    struct ndp_header header;
    uint32_t reserved;
    // Options follow...
};

/* NDP Router Advertisement */
struct ndp_router_advertisement {
    struct ndp_header header;
    uint8_t  cur_hop_limit;
    uint8_t  flags;
    uint16_t router_lifetime;
    uint32_t reachable_time;
    uint32_t retrans_timer;
    // Options follow...
};

/* NDP Neighbor Solicitation */
struct ndp_neighbor_solicitation {
    struct ndp_header header;
    uint32_t reserved;
    uint8_t  target_address[16]; // IPv6 address
    // Options follow...
};

/* NDP Neighbor Advertisement */
struct ndp_neighbor_advertisement {
    struct ndp_header header;
    uint32_t flags;
    uint8_t  target_address[16]; // IPv6 address
    // Options follow...
};

/* NDP Redirect */
struct ndp_redirect {
    struct ndp_header header;
    uint32_t reserved;
    uint8_t  target_address[16]; // IPv6 address
    uint8_t  destination_address[16]; // IPv6 address
    // Options follow...
};

/* NDP Option format */
struct ndp_option {
    uint8_t type;   /* Type of the option */
    uint8_t length; /* Length of the option in units of 8 octets */
    // Option Data follows...
};

/* Function declarations */
// Functions to parse and process NDP messages would be defined here.

#endif // __NDP_H__
