#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>

#include <netstack/ip.h>
#include <netstack/ndp.h>
#include <netstack/ethernet.h>
#include <netstack/udp.h>
#include <netstack/tcp.h>

errval_t ipv6_unmarshal(
    IP* ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(ip);

    struct ipv6_hdr* packet = (struct ipv6_hdr*)buf.data;
    uint8_t next_header = packet->next_header;
    uint16_t payload_len = ntohs(packet->len);

    // 1. Check the traffic class and flow label
    if (packet->vtf.tfclas != 0) {
        IP6_ERR("We Don't Support traffic class %d, but I'll ignore it for now", packet->vtf.tfclas);
        // return NET_ERR_IPv6_WRONG_FIELD;
    }
    if (packet->vtf.flabel != 0) {
        IP6_ERR("We Don't Support flow label %d, but I'll ignore it for now", packet->vtf.flabel);
        // return NET_ERR_IPv6_WRONG_FIELD;
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
    if (packet->dest != ip->my_ipv6) {
        char dest_ip_str[39]; format_ipv6_addr(packet->dest, dest_ip_str, sizeof(dest_ip_str));
        char my_ip_str[39]; format_ipv6_addr(ip->my_ipv6, my_ip_str, sizeof(my_ip_str));
        IP6_ERR("This IPv6 packet is for %s, not for me %s", dest_ip_str, my_ip_str);
        return NET_ERR_IPv6_WRONG_FIELD;
    }

    const ip_context_t src_ip_context = {
        .is_ipv6 = true,
        .ipv6    = packet->src,
    };

    // 6. Check the next header
    switch (next_header) {
        case IPv6_PROTO_UDP:
            IP6_DEBUG("UDP packet received");
            err = udp_unmarshal(ip->udp, src_ip_context, buf);
            DEBUG_FAIL_RETURN(err, "Can't unmarshal the UDP packet");
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

errval_t ipv6_marshal(
    IP* ip, ipv6_addr_t dst_ip, uint8_t proto, Buffer buf
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    assert(ip);

    // 1. Create the IPv6 header
    struct ipv6_hdr* packet = (struct ipv6_hdr*)buf.data;
    *packet = (struct ipv6_hdr) {
        .vtf        = {
            .version = 6,
            .tfclas  = 0,
            .flabel  = 0,
        },
        .len         = htonl((uint32_t)buf.valid_size),
        .next_header = proto,
        .hop_limit   = 0xFF,
        .src         = ip->my_ipv6,
        .dest        = ip->my_ipv6,
    };
    packet->vtf_uint32 = htonl(packet->vtf_uint32);
    
    // TODO: Next Header

    // 2. Get the destination MAC
    mac_addr dst_mac = MAC_NULL;
    err = ndp_lookup_mac(ip->ndp, dst_ip, &dst_mac);
    // switch (err_no(err))
    // {
    // case NET_ERR_NDP_NO_MAC_ADDRESS: 
    // {
    //     submit_delayed_task(MK_DELAY_TASK(msg->retry_interval, close_sending_message, MK_NORM_TASK(check_get_mac, (void*)msg)));
    //     return NET_THROW_SUBMIT_EVENT;
    // }
    // case SYS_ERR_OK: { // Continue sending
    //     assert(!(maccmp(dst_mac, MAC_NULL) || maccmp(dst_mac, MAC_BROADCAST)));
    //     msg->dst_mac = dst_mac;
        
    //     check_send_message((void*)msg);
    //     return NET_THROW_SUBMIT_EVENT;
    // }
    // default: 
    //     DEBUG_ERR(err, "Can't establish binding for given IP address");
    //     return err;
    // }

    return err;
}