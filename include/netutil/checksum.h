#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <stdint.h>

/**
 * Calculate the internet checksum according to RFC1071
 */
uint16_t inet_checksum(void *dataptr, uint16_t len);

#include <netutil/ip.h>

struct pseudo_ip_header_in_net_order {
    union {
        struct {
            uint32_t src_addr;
            uint32_t dst_addr;
            uint8_t  reserved;  // This is typically 0
            uint8_t  protocol;
            uint16_t len_no_iph;
        }__attribute__((packed)) ipv4;
        struct {
            ipv6_addr_t src_addr;
            ipv6_addr_t dst_addr;
            uint32_t    len_no_iph;   
            uint8_t     zero[3];      // Reserved 3-byte field set to zero
            uint8_t     next_header;  // Next Header field identifying the TCP protocol (8 bits)
        } __attribute__((packed)) ipv6;
    };
    bool is_ipv6;
} __attribute__((packed));

static_assert(sizeof(((struct pseudo_ip_header_in_net_order *)0)->ipv4) == 12, "Size of ipv4 sub-structure is not as expected (12)");
static_assert(sizeof(((struct pseudo_ip_header_in_net_order *)0)->ipv6) == 40, "Size of ipv6 sub-structure is not as expected (40)");

/**
 * Calculate the UDP checksum according to RFC1071
 */
uint16_t udp_checksum_in_net_order(const void *data_no_iph, struct pseudo_ip_header_in_net_order pheader);

/**
 * Calculate the TCP checksum according to RFC1071
 */
uint16_t tcp_checksum_in_net_order(const void *data_no_iph, struct pseudo_ip_header_in_net_order pheader);

#define PSEUDO_HEADER_IPv4(ipv4_src, ipv4_dst, proto, len) \
    (struct pseudo_ip_header_in_net_order) { \
        .ipv4 = { \
            .src_addr    = htonl(ipv4_src), \
            .dst_addr    = htonl(ipv4_dst), \
            .reserved    = 0, \
            .protocol    = proto, \
            .len_no_iph  = htons((uint16_t)len), \
        }, \
        .is_ipv6 = false, \
    }

#define PSEUDO_HEADER_IPv6(ipv6_src, ipv6_dst, next, len) \
    (struct pseudo_ip_header_in_net_order) { \
        .ipv6 = { \
            .src_addr    = hton16(ipv6_src), \
            .dst_addr    = hton16(ipv6_dst), \
            .len_no_iph  = htonl((uint32_t)len), \
            .zero        = { 0 }, \
            .next_header = next, \
        }, \
        .is_ipv6 = true, \
    }

#endif

