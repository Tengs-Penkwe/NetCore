#include <netstack/icmp.h>
#include <netutil/checksum.h>
#include <netutil/htons.h>
#include <netutil/dump.h>
#include <netutil/icmpv6.h>
#include <netstack/ndp.h> 

errval_t icmpv6_marshal(
    ICMP* icmp, ipv6_addr_t dst_ip, uint8_t type, uint8_t code, Buffer buf
) {
    assert(icmp); errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    buffer_sub_ptr(&buf, sizeof(struct icmpv6_hdr));
    struct icmpv6_hdr* packet = (struct icmpv6_hdr*) buf.data;

    switch (type) {
    case ICMPv6_NSA:
        *packet = (struct icmpv6_hdr) {
            .type   = type,
            .code   = code,
            .chksum = 0,    
        };
        err = SYS_ERR_OK;
        break;
    default:
        ICMP_ERR("WRONG ICMP type :%d!", type);
        return NET_ERR_ICMP_WRONG_TYPE;
    }

    // For ICMP, the checksum is calculated for the full packet
    struct pseudo_ip_header_in_net_order ip_header = PSEUDO_HEADER_IPv6(icmp->ip->my_ipv6, dst_ip, IP_PROTO_ICMPv6, buf.valid_size);
    packet->chksum = tcp_checksum_in_net_order(packet, ip_header);
    
    const ip_context_t dst_ip_context = {
        .is_ipv6 = true,
        .ipv6    = dst_ip,
    };
    err = ip_marshal(icmp->ip, dst_ip_context, IP_PROTO_ICMPv6, buf);
    DEBUG_FAIL_RETURN(err, "Can't send the ICMP through binding");
    return err;
}

errval_t icmpv6_unmarshal(
    ICMP* icmp, ipv6_addr_t src_ip, ipv6_addr_t dst_ip, Buffer buf
) {
    assert(icmp); errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    struct icmpv6_hdr *packet = (struct icmpv6_hdr*)buf.data;

    // 1. Check the checksum
    struct pseudo_ip_header_in_net_order ip_header = PSEUDO_HEADER_IPv6(src_ip, dst_ip, IP_PROTO_ICMPv6, buf.valid_size);
    if (tcp_checksum_in_net_order((void*)packet, ip_header) != 0) {
        return NET_ERR_ICMP_WRONG_CHECKSUM;
    }

    // Move the buffer pointer forward to next header
    buffer_add_ptr(&buf, sizeof(struct icmpv6_hdr));

    uint8_t type = packet->type;
    uint8_t code = packet->code;

    switch (type)
    {
    case ICMPv6_MLPv2:
        if (code != 0) return NET_ERR_ICMPv6_WRONG_CODE;

        struct mlpv2_hdr *mlpv2 = (struct mlpv2_hdr*)buf.data;
        uint16_t num_records = ntohs(mlpv2->num_records);

        // Move forward to the first record
        struct mutlicast_address_record* records = malloc(sizeof(struct mutlicast_address_record) * num_records);

        for (uint16_t i = 0; i < num_records; i++) {
            buffer_add_ptr(&buf, sizeof(struct mlpv2_hdr));
            records[i] = *(struct mutlicast_address_record*)buf.data;
            assert(records[i].aux_data_len == 0 && "The should return an error code");
        }
        LOG_INFO("MLPv2: %d records", num_records);

        err = SYS_ERR_OK;
        break;
    case ICMPv6_NSL:
        return ndp_neighbor_solicitation(icmp, src_ip, code, buf);
    default:
        pbuf(buf.data, buf.valid_size + 4, 8);
        return NET_ERR_ICMPv6_WRONG_TYPE;
    }

    return err;
}