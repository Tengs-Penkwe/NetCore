#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>

#include <netstack/ip.h>
#include <netstack/ndp.h>
#include <netstack/ethernet.h>
#include <netstack/udp.h>
#include <netstack/tcp.h>

#include "ip_slice.h"   // IP_send

errval_t ipv6_unmarshal(
    IP* ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(ip);

    struct ipv6_hdr* packet = (struct ipv6_hdr*)buf.data;
    uint16_t payload_len = ntohs(packet->len);
    ipv6_addr_t src_ip = ntoh16(packet->src);
    ipv6_addr_t dst_ip = ntoh16(packet->dest);

    // 1.1 Check the traffic class and flow label
    if (IP6H_TC(packet) != 0) {
        IP6_WARN("We Don't Support traffic class %d, but I'll ignore it for now", IP6H_TC(packet));
        // return NET_ERR_IPv6_WRONG_FIELD;
    }
    if (IP6H_FL(packet) != 0) {
        IP6_WARN("We Don't Support flow label %d, but I'll ignore it for now", IP6H_FL(packet));
        // return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 1.3 Check the hop limit
    if (packet->hop_limit == 0) {
        IP6_ERR("Invalid IPv6 packet hop limit %d", packet->hop_limit);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 1.4 Check the destination address
    if (dst_ip != ip->my_ipv6 && !ipv6_is_multicast(dst_ip)) {
        char dest_ip_str[39]; format_ipv6_addr(dst_ip, dest_ip_str, sizeof(dest_ip_str));
        char my_ip_str[39]; format_ipv6_addr(ip->my_ipv6, my_ip_str, sizeof(my_ip_str));
        IP6_ERR("This IPv6 packet is for %s, not for me %s", dest_ip_str, my_ip_str);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    const ip_context_t src_ip_context = {
        .is_ipv6 = true,
        .ipv6    = src_ip,
    };

    // 2. Check the extension headers
    uint8_t next_header = packet->next_header;
    uint8_t ext_length = 0;
    buffer_add_ptr(&buf, sizeof(struct ipv6_hdr));

    // 1.2 Check the payload length
    if (payload_len != buf.valid_size) {
        IP6_ERR("The payload length %d do not match with the buffer size %d", payload_len, buf.valid_size);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

jump_next:

    switch (next_header)
    {
    case IP_PROTO_HOPOPT: 
        struct ipv6_hopbyhop_hdr *hop_hdr = (struct ipv6_hopbyhop_hdr *)buf.data;

        next_header = hop_hdr->next_header;
        ext_length  = hop_hdr->length;

        NDP_WARN("Hopbyhop options Not Implemented Yet");

        // TODO: this is not correct if multiple extension headers are present
        uint8_t hdr_with_padding = ROUND_UP(sizeof(struct ipv6_hopbyhop_hdr) + ext_length, 8);
        buffer_add_ptr(&buf, hdr_with_padding); 
        goto jump_next;
    case IP_PROTO_UDP:
        IP6_DEBUG("UDP packet received");
        err = udp_unmarshal(ip->udp, src_ip_context, buf);
        DEBUG_FAIL_RETURN(err, "Can't unmarshal the UDP packet");
        break;
    case IP_PROTO_TCP:
        IP6_DEBUG("TCP packet received");
        // err = tcp_unmarshal(ip->tcp, buf);
        // DEBUG_FAIL_RETURN(err, "Can't unmarshal the TCP packet");
        break;
    case IP_PROTO_ICMPv6:
        IP6_DEBUG("ICMP packet received");
        // the destination address may be a multicast address, like 'ff02::1:ffdc:6aa7'
        err = icmpv6_unmarshal(ip->icmp, src_ip, dst_ip, buf);
        DEBUG_FAIL_RETURN(err, "Can't unmarshal the ICMP packet");
        break;
    default:
        IP6_ERR("Invalid IPv6 packet next header %d", next_header);
        return NET_ERR_IPv6_NEXT_HEADER;
    }

    return err;
}

errval_t ipv6_send(
    IP* ip, const ipv6_addr_t dst_ip, const mac_addr dst_mac,
    const uint8_t proto, Buffer buf
) {
    assert(ip); errval_t err = SYS_ERR_OK;
    
    // 1. Prepare the send buffer
    buffer_sub_ptr(&buf, sizeof(struct ipv6_hdr));

    // 2. Fill the header
    struct ipv6_hdr* packet = (struct ipv6_hdr*)buf.data;
    *packet = (struct ipv6_hdr) {
        .vtc_flow    = htonl(IP6H_VTCFLOW(6, 0, 0)),
        .len         = htons(buf.valid_size - sizeof(struct ipv6_hdr)),
        .next_header = proto,
        .hop_limit   = 0xFF,
        .src         = hton16(ip->my_ipv6),
        .dest        = hton16(dst_ip),
    };

    // 3. Send the packet
    err = ethernet_marshal(ip->ether, dst_mac, ETH_TYPE_IPv6, buf);
    DEBUG_FAIL_RETURN(err, "Can't send the IPv4 packet");

    IP_VERBOSE("End sending an IPv6 packet with size: %d, proto: %d", buf.valid_size, proto);
    return err;
}
