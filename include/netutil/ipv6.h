#ifndef _IPV6_H_
#define _IPV6_H_

#include <net.h>

// Structure of an IPv6 header
struct ipv6_hdr {
    uint32_t vtc_fl; // Version, Traffic Class, and Flow Label
    uint16_t len;    // Payload Length
    uint8_t next_header; // Next Header
    uint8_t hop_limit;   // Hop Limit
    long long src; // Source IPv6 address
    long long dest; // Destination IPv6 address
} __attribute__((__packed__));

#endif // _IPV6_H_
