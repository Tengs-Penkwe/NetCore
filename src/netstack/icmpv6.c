#include <netstack/icmp.h>
#include <netutil/checksum.h>
#include <netutil/htons.h>
#include <netutil/dump.h>
#include <netutil/icmpv6.h>
#include <netstack/ndp.h> 

errval_t icmpv6_unmarshal(
    ICMP* icmp, ipv6_addr_t src_ip, ipv6_addr_t dst_ip, Buffer buf
) {
    assert(icmp); errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    struct icmpv6_hdr *packet = (struct icmpv6_hdr*)buf.data;

    // 1. Check the checksum
    struct pseudo_ip_header_in_net_order ip_header = PSEUDO_HEADER_IPv6(src_ip, dst_ip, IPv6_PROTO_ICMP, buf.valid_size);
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

        LOG_ERR("MLPv2: %d records\n", num_records);

        err = SYS_ERR_OK;
        break;
    case ICMPv6_NSL:
        if (code != 0) return NET_ERR_ICMPv6_WRONG_CODE;

        struct ndp_neighbor_solicitation *nsl = (struct ndp_neighbor_solicitation*)buf.data;
    pbuf(nsl, buf.valid_size + 4, 8);

        // TODO: test if the IP is multicast (RFC 4861), if so, return NET_ERR_NDP_WRONG_DESTINATION
        if (ntoh16(nsl->to_addr) != icmp->ip->my_ipv6) return NET_ERR_NDP_WRONG_DESTINATION;

        for (uint16_t i = 0; i < 3; i++) {
            struct ndp_option *option = (struct ndp_option*)buf.data;
            buffer_add_ptr(&buf, option->length * 8);
            switch (option->type)
            {
            case NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS:
                ndp_register(icmp, src_ip, ntoh6(mem2mac(option->data)));
                return err;
            case NDP_OPTION_TARGET_LINK_LAYER_ADDRESS:
            case NDP_OPTION_PREFIX_INFORMATION:
            case NDP_OPTION_REDIRECTED_HEADER:
            case NDP_OPTION_MTU:
                NDP_WARN("NOT implemented NSL: %d\n", option->type);
                continue;
            default:
                NDP_ERR("Unknown NDP option type: %d\n", option->type);
                return NET_ERR_NDP_UNKNOWN_OPTION;
            }
        }
        err = SYS_ERR_OK;
        break;
    default: return NET_ERR_ICMPv6_WRONG_TYPE;
    }


    return err;
}