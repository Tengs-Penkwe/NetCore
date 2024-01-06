#ifndef __NETUTIL_DUMP_H__
#define __NETUTIL_DUMP_H__

#include <netutil/etharp.h>
#include <netutil/ip.h>
#include <netutil/icmp.h>
#include <netutil/udp.h>
#include <netutil/tcp.h>
#include <netutil/ip.h>

#include <stdio.h>  //printf
                    
__BEGIN_DECLS

static inline void pbufrow(const void* buffer, size_t length) {
    for (size_t index = 0; index < length; index += 1) {
        printf("%02X", ((const uint8_t*) buffer)[index]);
    }
    printf("\n");
}

static inline void pbuf(const void* buffer, size_t length, size_t column) {
    const uint8_t* ptr = (const uint8_t*) buffer;
    size_t nrows = length / column;
    for (size_t row = 0; row < nrows; row += 1) {
        printf("[%04lu] ", row * column);
        pbufrow(ptr, column);
        ptr += column;
    }
    if (nrows * column < length) {
        printf("[%04lu] ", nrows * column);
        pbufrow(ptr, length - nrows * column);
    }
}

int format_mac_address(const mac_addr *addr, char *buffer, size_t max_len);
int format_ip_addr(ip_context_t ip, char *buffer, size_t max_len);
int format_ipv4_addr(ip_addr_t ip, char *buffer, size_t max_len);
int format_ipv6_addr(ipv6_addr_t addr, char *buffer, size_t max_len);
int format_tcp_flags(uint8_t flags, char *buffer, size_t max_len);
int format_tcp_header(const struct tcp_hdr *tcp_header, char *buffer, size_t max_len);
int format_udp_header(const struct udp_hdr *udp_header, char *buffer, size_t max_len);
int format_ethernet_header(const struct eth_hdr *eth_header, char *buffer, size_t max_len);
int format_icmp_header(const struct icmp_hdr *icmp_header, char *buffer, size_t max_len);
int format_arp_header(const struct arp_hdr *arp_header, char *buffer, size_t max_len);
int format_ipv4_header(const struct ip_hdr *ip_header, char *buffer, size_t max_len);
int format_ipv6_header(const struct ipv6_hdr *ipv6_header, char *buffer, size_t max_len);
int format_packet_info(const void *packet_start, char *buffer, size_t max_len);

static inline void dump_packet_info(const void* packet_start) {
    char buffer[1024];
    format_packet_info(packet_start, buffer, sizeof(buffer));
    printf("%s\n", buffer);
}

__END_DECLS

#endif  //__NETUTIL_DUMP_H__
