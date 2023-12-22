#ifndef __NETUTIL_DUMP_H__
#define __NETUTIL_DUMP_H__

#include <netutil/etharp.h>
#include <netutil/ip.h>
#include <netutil/icmp.h>
#include <netutil/udp.h>
#include <netutil/tcp.h>
#include <netutil/ipv6.h>

__BEGIN_DECLS
    
// Helper function to print MAC address
void print_mac_address(const mac_addr *addr);

// Helper function to print IP address in human-readable format
void print_ip_address(ip_addr_t ip);

void dump_ethernet_header(const struct eth_hdr *eth_header);
void dump_arp_header(const struct arp_hdr *arp_header);
void print_tcp_flags(uint8_t flags);
void dump_tcp_header(const struct tcp_hdr *tcp_header);
void dump_ipv4_header(const struct ip_hdr *ip_header);
void dump_ipv6_header(const struct ipv6_hdr *ipv6_header);
void dump_icmp_header(const struct icmp_hdr *icmp_header);
void dump_udp_header(const struct udp_hdr *udp_header);

// Function to dump packet information
void dump_packet_info(const void *packet_start);

__END_DECLS

#endif  //__NETUTIL_DUMP_H__