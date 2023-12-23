#include <netutil/dump.h>
#include <arpa/inet.h>
#include <sys/socket.h>  //AF_INET6

// Helper function to print MAC address
void print_mac_address(const mac_addr *addr) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
        addr->addr[0], addr->addr[1], addr->addr[2], 
        addr->addr[3], addr->addr[4], addr->addr[5]);
}

// Helper function to print IP address in human-readable format
void print_ip_address(ip_addr_t ip) {
    struct in_addr ip_addr;
    ip_addr.s_addr = ip;
    printf("%s\n", inet_ntoa(ip_addr));
}

// Helper function to print TCP flags in a readable format
void print_tcp_flags(uint8_t flags) {
    printf("   Flags:");
    if (tcp_flag_is_set(flags, TCP_FIN)) printf(" FIN");
    if (tcp_flag_is_set(flags, TCP_SYN)) printf(" SYN");
    if (tcp_flag_is_set(flags, TCP_RST)) printf(" RST");
    if (tcp_flag_is_set(flags, TCP_PSH)) printf(" PSH");
    if (tcp_flag_is_set(flags, TCP_ACK)) printf(" ACK");
    if (tcp_flag_is_set(flags, TCP_URG)) printf(" URG");
    if (tcp_flag_is_set(flags, TCP_ECE)) printf(" ECE");
    if (tcp_flag_is_set(flags, TCP_CWR)) printf(" CWR");
    printf("\n");
}

// Function to dump TCP header
void dump_tcp_header(const struct tcp_hdr *tcp_header) {
    printf("TCP Header:\n");
    printf("   Source Port: %u\n", ntohs(tcp_header->src_port));
    printf("   Destination Port: %u\n", ntohs(tcp_header->dest_port));
    printf("   Sequence Number: %u\n", ntohl(tcp_header->seqno));
    printf("   Acknowledgment Number: %u\n", ntohl(tcp_header->ackno));
    printf("   Data Offset: %u bytes\n", tcp_get_data_offset(tcp_header));
    print_tcp_flags(tcp_header->flags);
    printf("   Window Size: %u\n", ntohs(tcp_header->window));
    printf("   Checksum: 0x%04x\n", ntohs(tcp_header->chksum));
    printf("   Urgent Pointer: %u\n", ntohs(tcp_header->urgent_ptr));
}

void dump_ethernet_header(const struct eth_hdr *eth_header) {
    printf("Ethernet Header:\n");
    printf("   Destination MAC: ");
    print_mac_address(&eth_header->dst);
    printf("   Source MAC: ");
    print_mac_address(&eth_header->src);
    printf("   EtherType: 0x%04x\n", ntohs(eth_header->type));
}

void dump_icmp_header(const struct icmp_hdr *icmp_header) {
    printf("ICMP Header:\n");
    printf("   Type: %u\n", icmp_header->type);
    printf("   Code: %u\n", icmp_header->code);
    printf("   Checksum: 0x%04x\n", ntohs(icmp_header->chksum));
    // Add more fields as necessary
}

void dump_arp_header(const struct arp_hdr *arp_header) {
    printf("ARP Header:\n");
    printf("   Hardware Type: %u\n", ntohs(arp_header->hwtype));
    printf("   Protocol Type: 0x%04x\n", ntohs(arp_header->proto));
    printf("   Hardware Size: %u\n", arp_header->hwlen);
    printf("   Protocol Size: %u\n", arp_header->protolen);
    printf("   Opcode: %u\n", ntohs(arp_header->opcode));
    printf("   Sender MAC: ");
    print_mac_address(&arp_header->eth_src);
    printf("   Sender IP: ");
    print_ip_address(arp_header->ip_src);
    printf("   Target MAC: ");
    print_mac_address(&arp_header->eth_dst);
    printf("   Target IP: ");
    print_ip_address(arp_header->ip_dst);
}

void dump_ipv4_header(const struct ip_hdr *ip_header) {
    printf("IPv4 Header:\n");
    printf("   IP Version: %u\n", IPH_V(ip_header));
    printf("   Header Length: %u bytes\n", IPH_HL(ip_header));
    printf("   Type of Service: %u\n", ip_header->tos);
    printf("   Total Length: %u\n", ntohs(ip_header->len));
    printf("   Identification: %u\n", ntohs(ip_header->id));
    printf("   Flags: %u\n", ntohs(ip_header->offset) >> 13);
    printf("   Fragment Offset: %u\n", ntohs(ip_header->offset) & IP_OFFMASK);
    printf("   Time to Live: %u\n", ip_header->ttl);
    printf("   Protocol: %u\n", ip_header->proto);
    printf("   Checksum: 0x%04x\n", ntohs(ip_header->chksum));
    printf("   Source IP: ");
    print_ip_address(ip_header->src);
    printf("   Destination IP: ");
    print_ip_address(ip_header->dest);

    switch (ip_header->proto) {
    case IP_PROTO_ICMP:
        dump_icmp_header((const struct icmp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)));
        break;
    case IP_PROTO_UDP: // This constant may vary depending on your system headers
        dump_udp_header((const struct udp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)));
        break;
    case IP_PROTO_TCP:
        dump_tcp_header((const struct tcp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)));
        break;
    default:
        printf("Unknown IPv4 Header: %04x\n", ip_header->proto);
    }
}

void dump_ipv6_header(const struct ipv6_hdr *ipv6_header) {
    printf("IPv6 Header:\n");
    // Add more detailed IPv6 header information here
    char src_ip[INET6_ADDRSTRLEN];
    char dest_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ipv6_header->src, src_ip, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &ipv6_header->dest, dest_ip, INET6_ADDRSTRLEN);
    printf("   Source IP: %s\n", src_ip);
    printf("   Destination IP: %s\n", dest_ip);
    printf("   Next Header: %u\n", ipv6_header->next_header);
    printf("   Hop Limit: %u\n", ipv6_header->hop_limit);
}

void dump_udp_header(const struct udp_hdr *udp_header) {
    printf("UDP Header:\n");
    printf("   Source Port: %u\n", ntohs(udp_header->src));
    printf("   Destination Port: %u\n", ntohs(udp_header->dest));
    printf("   Length: %u\n", ntohs(udp_header->len));
    printf("   Checksum: 0x%04x\n", ntohs(udp_header->chksum));
}

// Function to dump packet information
void dump_packet_info(const void *packet_start) {
    const struct eth_hdr *eth_header = (const struct eth_hdr *)packet_start;
    dump_ethernet_header(eth_header);

    switch(ntohs(eth_header->type)) {
    case ETH_TYPE_ARP:
        dump_arp_header((const struct arp_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)));
        break;
    case ETH_TYPE_IPv4:
        dump_ipv4_header((const struct ip_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)));
        break;
    case ETH_TYPE_IPv6:
        dump_ipv6_header((const struct ipv6_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)));
        break;
    default:
        printf("Unknown Header:\n");
    }
}
