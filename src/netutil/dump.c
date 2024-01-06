#include <netutil/dump.h>
#include <netutil/ip.h>
#include <netutil/htons.h>

int format_mac_address(const mac_addr *addr, char *buffer, size_t max_len) {
    return snprintf(buffer, max_len, "%02X:%02X:%02X:%02X:%02X:%02X",
                    addr->addr[0], addr->addr[1], addr->addr[2], 
                    addr->addr[3], addr->addr[4], addr->addr[5]);
}

int format_ip_addr(ip_context_t ip, char *buffer, size_t max_len) {
    if (ip.is_ipv6) {
        return format_ipv6_addr(ip.ipv6, buffer, max_len);
    } else {
        return format_ipv4_addr(ip.ipv4, buffer, max_len);
    }
}

#include <stdio.h>

int format_ipv6_addr(ipv6_addr_t addr, char *buffer, size_t max_len) {
    // Extracting each 16-bit block from the 128-bit address
    uint8_t subblocks[16];
    uint16_t blocks[8];
    for (int i = 0; i < 16; ++i) {
        subblocks[i] = (addr >> (120 - 8 * i)) & 0xFF;
        if (i % 2 == 1) {
            blocks[i / 2] = (subblocks[i - 1] << 8) | subblocks[i];
        }
    }
    return snprintf(buffer, max_len, "%x:%x:%x:%x:%x:%x:%x:%x", blocks[0], blocks[1], blocks[2],
                    blocks[3], blocks[4], blocks[5], blocks[6], blocks[7]);
}

int format_ipv4_addr(ip_addr_t ip, char *buffer, size_t max_len) {
    uint8_t blocks[4];
    for (int i = 0; i < 4; ++i) {
        blocks[i] = (ip >> (24 - 8 * i)) & 0xFF;
    }
    return snprintf(buffer, max_len, "%u.%u.%u.%u", blocks[0], blocks[1], blocks[2], blocks[3]);
}

int format_tcp_flags(uint8_t flags, char *buffer, size_t max_len) {
    int len = 0, n = 0;
    n = snprintf(buffer + len, max_len - len, "   Flags:");
    len += n;
    if (tcp_flag_is_set(flags, TCP_FIN)) { n = snprintf(buffer + len, max_len - len, " FIN"); }
    if (tcp_flag_is_set(flags, TCP_SYN)) { n = snprintf(buffer + len, max_len - len, " SYN"); }
    if (tcp_flag_is_set(flags, TCP_RST)) { n = snprintf(buffer + len, max_len - len, " RST"); }
    if (tcp_flag_is_set(flags, TCP_PSH)) { n = snprintf(buffer + len, max_len - len, " PSH"); }
    if (tcp_flag_is_set(flags, TCP_ACK)) { n = snprintf(buffer + len, max_len - len, " ACK"); }
    if (tcp_flag_is_set(flags, TCP_URG)) { n = snprintf(buffer + len, max_len - len, " URG"); }
    if (tcp_flag_is_set(flags, TCP_ECE)) { n = snprintf(buffer + len, max_len - len, " ECE"); }
    if (tcp_flag_is_set(flags, TCP_CWR)) { n = snprintf(buffer + len, max_len - len, " CWR"); }
    len += n;
    n = snprintf(buffer + len, max_len - len, "\n");
    return len += n;
}

int format_tcp_header(const struct tcp_hdr *tcp_header, char *buffer, size_t max_len) {
    int len = snprintf(buffer, max_len,
    "TCP Header:\n"
    "   Source Port: %u\n"
    "   Destination Port: %u\n" 
    "   Sequence Number: %u\n" 
    "   Acknowledgment Number: %u\n"
    "   Data Offset: %u bytes\n", 
    ntohs(tcp_header->src_port), ntohs(tcp_header->dest_port), ntohl(tcp_header->seqno), 
    ntohl(tcp_header->ackno), tcp_get_data_offset(tcp_header));

    // TCP Flags
    char flag_buffer[32];
    format_tcp_flags(tcp_header->flags, flag_buffer, sizeof(flag_buffer));
    len += snprintf(buffer + len, max_len - len,
     "%s"
     "   Window Size: %u\n"
     "   Checksum: 0x%04x\n"
     "   Urgent Pointer: %u\n", 
    flag_buffer, ntohs(tcp_header->window), ntohs(tcp_header->chksum), ntohs(tcp_header->urgent_ptr));
    
    return len;
}

int format_udp_header(const struct udp_hdr *udp_header, char *buffer, size_t max_len) {
    return snprintf(buffer, max_len,
    "UDP Header:\n"
    "   Source Port: %u\n" 
    "   Destination Port: %u\n"
    "   Length: %u\n" 
    "   Checksum: 0x%04x\n", 
    ntohs(udp_header->src), ntohs(udp_header->dest), ntohs(udp_header->len), ntohs(udp_header->chksum));
}

int format_ethernet_header(const struct eth_hdr *eth_header, char *buffer, size_t max_len) {
    int len = 0;
    len += snprintf(buffer + len, max_len - len, "Ethernet Header:\n");

    char mac_buffer[20];
    format_mac_address(&eth_header->dst, mac_buffer, sizeof(mac_buffer));
    len += snprintf(buffer + len, max_len - len, "   Destination MAC: %s\n", mac_buffer);
    format_mac_address(&eth_header->src, mac_buffer, sizeof(mac_buffer));
    len += snprintf(buffer + len, max_len - len, "   Source MAC: %s\n", mac_buffer);
    len += snprintf(buffer + len, max_len - len, "   EtherType: 0x%04x\n", ntohs(eth_header->type));

    return len;
}

int format_icmp_header(const struct icmp_hdr *icmp_header, char *buffer, size_t max_len) {
    return snprintf(buffer, max_len,
    "ICMP Header:\n"
    "   Type: %u\n"
    "   Code: %u\n"
    "   Checksum: 0x%04x\n",
    icmp_header->type, icmp_header->code, ntohs(icmp_header->chksum));
}

int format_arp_header(const struct arp_hdr *arp_header, char *buffer, size_t max_len) {
    int len = 0;

    len += snprintf(buffer + len, max_len - len, "ARP Header:\n");

    char mac_buffer[20], ip_buffer[20];
    format_mac_address(&arp_header->eth_src, mac_buffer, sizeof(mac_buffer));
    format_ipv4_addr(arp_header->ip_src, ip_buffer, sizeof(ip_buffer));
    len += snprintf(buffer + len, max_len - len,
                 "   Hardware Type: %u\n   Protocol Type: 0x%04x\n"
                 "   Hardware Size: %u\n   Protocol Size: %u\n"
                 "   Opcode: %u\n   Sender MAC: %s\n   Sender IP: %s\n",
                 ntohs(arp_header->hwtype), ntohs(arp_header->proto),
                 arp_header->hwlen, arp_header->protolen,
                 ntohs(arp_header->opcode), mac_buffer, ip_buffer);

    format_mac_address(&arp_header->eth_dst, mac_buffer, sizeof(mac_buffer));
    format_ipv4_addr(arp_header->ip_dst, ip_buffer, sizeof(ip_buffer));
    len += snprintf(buffer + len, max_len - len, "   Target MAC: %s\n   Target IP: %s\n", mac_buffer, ip_buffer);

    return len;
}

int format_ipv4_header(const struct ip_hdr *ip_header, char *buffer, size_t max_len) {
    char ip_src[IPv4_ADDRESTRLEN];
    char ip_dest[IPv4_ADDRESTRLEN];
    format_ipv4_addr(ip_header->src, ip_src, sizeof(ip_src));
    format_ipv4_addr(ip_header->dest, ip_dest, sizeof(ip_dest));

    int len = snprintf(buffer, max_len,
                       "IPv4 Header:\n"
                       "   IP Version: %u\n"
                       "   Header Length: %u bytes\n"
                       "   Type of Service: %u\n"
                       "   Total Length: %u\n"
                       "   Identification: %u\n"
                       "   Flags: %u\n"
                       "   Fragment Offset: %u\n"
                       "   Time to Live: %u\n"
                       "   Protocol: %u\n"
                       "   Checksum: 0x%04x\n"
                       "   Source IP: %s\n"
                       "   Destination IP: %s\n",
                       IPH_V(ip_header), IPH_HL(ip_header), ip_header->tos, ntohs(ip_header->total_len), 
                       ntohs(ip_header->id), ntohs(ip_header->offset) >> 13, ntohs(ip_header->offset) & IP_OFFMASK,
                       ip_header->ttl, ip_header->proto, ntohs(ip_header->chksum), ip_src, ip_dest);

    switch (ip_header->proto) {
    case IP_PROTO_ICMP:
        len += format_icmp_header((const struct icmp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)), buffer + len, max_len - len);
        break;
    case IP_PROTO_UDP: // This constant may vary depending on your system headers
        len += format_udp_header((const struct udp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)), buffer + len, max_len - len);
        break;
    case IP_PROTO_TCP:
        len += format_tcp_header((const struct tcp_hdr *)((const uint8_t *)ip_header + IPH_HL(ip_header)), buffer + len, max_len - len);
        break;
    default:
        len += snprintf(buffer + len, max_len - len, "Unknown IPv4 Header: %04x\n", ip_header->proto);
    }

    return len; // Number of characters written (excluding null terminator)
}

int format_ipv6_header(const struct ipv6_hdr *ipv6_header, char *buffer, size_t max_len) {
    char src_ip[IPv6_ADDRESTRLEN];
    char dest_ip[IPv6_ADDRESTRLEN];
    format_ipv6_addr(ntoh16(ipv6_header->src), src_ip, sizeof(src_ip));
    format_ipv6_addr(ntoh16(ipv6_header->dest), dest_ip, sizeof(dest_ip));

    // Extract version, traffic class, and flow label from the first 4 bytes
    uint32_t vtc_flow = ntohl(ipv6_header->vtc_flow);
    uint8_t version = (vtc_flow >> 28) & 0x0F;
    uint8_t traffic_class = (vtc_flow >> 20) & 0xFF;
    uint32_t flow_label = vtc_flow & 0xFFFFF;

    int len = snprintf(buffer, max_len,
                       "IPv6 Header:\n"
                       "   Version: %u\n"
                       "   Traffic Class: %u\n"
                       "   Flow Label: %u\n"
                       "   Payload Length: %u\n"
                       "   Next Header: %u\n"
                       "   Hop Limit: %u\n"
                       "   Source IP: %s\n"
                       "   Destination IP: %s\n",
                       version, traffic_class, flow_label,
                       ntohs(ipv6_header->payload_len),
                       ipv6_header->next_header, ipv6_header->hop_limit,
                       src_ip, dest_ip);

    // Handle the next header (extension headers if any)
    // const uint8_t *next_header = (const uint8_t *)(ipv6_header + 1); // pointer to next header
    // uint8_t next_header_type = ipv6_header->next_header;
    // size_t remaining_len = max_len - len;

    // Loop to process extension headers
    // while (is_extension_header(next_header_type)) {
    //     // Use a function that processes the specific extension header
    //     // This function should return the type of the next header
    //     // Example: next_header_type = process_extension_header(next_header, buffer + len, &remaining_len);
    //     // Update the next_header pointer and length accordingly
    //     // Example: next_header = update_next_header_pointer(next_header, next_header_type);
    // }

    return len;
}


int format_packet_info(const void *packet_start, char *buffer, size_t max_len) {
    int len = 0;
    const struct eth_hdr *eth_header = (const struct eth_hdr *)packet_start;

    len += format_ethernet_header(eth_header, buffer + len, max_len - len); // Assuming format_ethernet_header is implemented

    uint16_t type = ntohs(eth_header->type);
    switch (type) {
    case ETH_TYPE_ARP:
        len += format_arp_header((const struct arp_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)), buffer + len, max_len - len); 
        break;
    case ETH_TYPE_IPv4:
        len += format_ipv4_header((const struct ip_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)), buffer + len, max_len - len);
        break;
    case ETH_TYPE_IPv6:
        len += format_ipv6_header((const struct ipv6_hdr *)((const uint8_t *)packet_start + sizeof(struct eth_hdr)), buffer + len, max_len - len);
        break;
    default:
        len += snprintf(buffer + len, max_len - len, "Unknown Header: %d\n", type);
    }

    return len;
}
