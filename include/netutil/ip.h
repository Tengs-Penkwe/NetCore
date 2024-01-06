#ifndef _NETUTIL_IP_H_
#define _NETUTIL_IP_H_

#include <common.h>

/********************  IPv4  ********************
 ************************************************/

#define IP_RF 0x8000U        /* reserved fragment flag */
#define IP_DF 0x4000U        /* dont fragment flag */
#define IP_MF 0x2000U        /* more fragments flag */
#define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */
#define IP_LEN_MIN       20       /* minimum size of an ip header*/
#define IP_LEN_MAX       0xFFFF   /* maximum size of an ip header*/

#define OFFSET_RF_SET(offset, rf) (offset |= (((uint16_t)rf) << 15))
#define OFFSET_DF_SET(offset, df) (offset |= (((uint16_t)df) << 14))
#define OFFSET_MF_SET(offset, mf) (offset |= (((uint16_t)mf) << 13))

typedef uint32_t ip_addr_t;
#define MK_IP(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

#define IPH_LEN_MIN  20
#define IPH_LEN_MAX  60

#define IPH_V(hdr)  ((hdr)->version)
#define IPH_HL(hdr) ((hdr)->ihl * 4)

struct ip_hdr {
    uint8_t   ihl     : 4; /* header length */
    uint8_t   version : 4;
    uint8_t   tos;       /* type of service */
    uint16_t  total_len; /* total length */
    uint16_t  id;        /* identification */
    uint16_t  offset;    /* fragment offset field */
    uint8_t   ttl;       /* time to live */
    uint8_t   proto;
    uint16_t  chksum;
    ip_addr_t src;
    ip_addr_t dest;
} __attribute__((__packed__));

/********************  IPv6  ********************
 ************************************************/

#define IPv4_ADDRESTRLEN   16
#define IPv6_ADDRESTRLEN   46

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
typedef unsigned __int128 ipv6_addr_t ;
#pragma GCC diagnostic pop
static_assert(sizeof(ipv6_addr_t) == 16, "ipv6_addr_t must be 16 bytes");

// Structure of an IPv6 header
struct ipv6_hdr {
    // uint32_t    version : 4;   // Version
    // uint32_t    tfclas  : 8;   // Traffic Class
    // uint32_t    flabel  : 20;  // Flow Label
    uint32_t    vtc_flow;
    uint16_t    payload_len;  // Payload Length
    uint8_t     next_header;  // Next Header
    uint8_t     hop_limit;    // Hop Limit
    ipv6_addr_t src;          // Source IPv6 address
    ipv6_addr_t dest;         // Destination IPv6 address
} __attribute__((__packed__));
#define IP6H_V(hdr)  ( htonl((hdr)->vtc_flow) >> 28)
#define IP6H_TC(hdr) ((htonl((hdr)->vtc_flow) >> 20) & 0xFF)
#define IP6H_FL(hdr) ( htonl((hdr)->vtc_flow) & 0x00FFFFF)
#define IP6H_VTCFLOW(version, tfclas, flabel) (((version) << 28) | ((tfclas) << 20) | (flabel))

static_assert(sizeof(struct ipv6_hdr) == 40, "IPv6 header must be 40 bytes");

typedef struct ipv6_hopbyhop_hdr {
    uint8_t   next_header;  // Next Header
    uint8_t   length;       // Length
    uint8_t   options[6];    // Options
} __attribute__((__packed__)) ipv6_hopbyhop_hdr_t;

static_assert(sizeof(struct ipv6_hopbyhop_hdr) == 8, "IPv6 Hop-by-Hop header must be 8 bytes");

typedef struct ipv6_routing_hdr {
    uint8_t   next_header;  // Next Header
    uint8_t   length;       // Length
    uint8_t   routing_type; // Routing Type
    uint8_t   segments_left;// Segments Left
    uint8_t   data[];       // Routing Data
} __attribute__((__packed__)) ipv6_routing_hdr_t;

/*******************  Common  ********************
 ************************************************/
#define IP_PROTO_HOPOPT    0
#define IP_PROTO_ICMP      1
#define IP_PROTO_IGMP      2
#define IP_PROTO_TCP       6
#define IP_PROTO_UDP       0x11     // 17: UDP
#define IP_PROTO_IPv6_FRAG 0x2c     // 43: IPv6 Fragmentation Header
#define IP_PROTO_UDPLITE   0x88     // 136: UDP-Lite (RFC 3828)
#define IP_PROTO_ICMPv6    0x3A     // 58: ICMPv6

typedef struct ip_context {
    bool    is_ipv6;
    union {
        ip_addr_t   ipv4;
        ipv6_addr_t ipv6;
    };
} ip_context_t;

__BEGIN_DECLS

static inline ipv6_addr_t mk_ipv6(uint64_t hi, uint64_t lo) {
    return ((ipv6_addr_t)hi << 64) | lo;
}

static inline bool ipv6_is_multicast(ipv6_addr_t addr) {
    return addr >> 120 == 0xFF;
}

__END_DECLS

#endif // _IP_H_
