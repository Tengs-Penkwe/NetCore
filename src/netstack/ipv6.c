#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>

#include <netstack/ipv6.h>
#include <netstack/ndp.h>
#include <netstack/ethernet.h>
#include <netstack/udp.h>
#include <netstack/tcp.h>

errval_t ipv6_init(
    IPv6* ip, ipv6_addr_t my_ip, Ethernet* ether, NDP* ndp, UDP* udp, TCP* tcp
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    ip->my_ip = my_ip;
    ip->ether = ether;
    ip->ndp = ndp;
    ip->udp = udp;
    ip->tcp = tcp;

    return err;
}

void ipv6_destroy(
    IPv6* ip
) {
    if (ip->ether != NULL) {
        ethernet_destroy(ip->ether);
    }
    if (ip->ndp != NULL) {
        ndp_destroy(ip->ndp);
    }
    if (ip->udp != NULL) {
        udp_destroy(ip->udp);
    }
    if (ip->tcp != NULL) {
        tcp_destroy(ip->tcp);
    }
    IP6_NOTE("IPv6 Module Destroyed");
}

errval_t ipv6_unmarshal(
    IPv6* ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(ip && buf.data);

    struct ipv6_hdr* packet = (struct ipv6_hdr*)buf.data;
    uint8_t next_header = packet->next_header;
    uint16_t payload_len = ntohs(packet->len);

    // 1. Check the version
    if (packet->version != 6) {
        IP6_ERR("Invalid IPv6 packet version %d", packet->version);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 2. Check the payload length
    if (payload_len != buf.valid_size) {
        IP6_ERR("The payload length %d do not match with the buffer size %d", payload_len, buf.valid_size);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 3. Check the hop limit
    if (packet->hop_limit == 0) {
        IP6_ERR("Invalid IPv6 packet hop limit %d", packet->hop_limit);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 4. Check the destination address
    if (packet->dest != ip->my_ip) {
        char dest_ip_str[39]; format_ipv6_addr(packet->dest, dest_ip_str, sizeof(dest_ip_str));
        char my_ip_str[39]; format_ipv6_addr(ip->my_ip, my_ip_str, sizeof(my_ip_str));
        IP6_ERR("This IPv6 packet is for %s, not for me %s", dest_ip_str, my_ip_str);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    // 6. Check the next header
    switch (next_header) {
        case IPv6_PROTO_UDP:
            IP6_DEBUG("UDP packet received");
            // err = udp_unmarshal(ip->udp, buf);
            // DEBUG_FAIL_RETURN(err, "Can't unmarshal the UDP packet");
            break;
        case IPv6_PROTO_TCP:
            IP6_DEBUG("TCP packet received");
            // err = tcp_unmarshal(ip->tcp, buf);
            // DEBUG_FAIL_RETURN(err, "Can't unmarshal the TCP packet");
            break;
        case IPv6_PROTO_ICMP:
            IP_DEBUG("ICMP packet received");
            // err = icmp_unmarshal(ip->icmp, buf);
            // DEBUG_FAIL_RETURN(err, "Can't unmarshal the ICMP packet");
            break;
        default:
            IP6_ERR("Invalid IPv6 packet next header %d", next_header);
            return NET_ERR_IPv6_NEXT_HEADER;
    }

    return err;
}