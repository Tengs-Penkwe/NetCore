#include <netutil/htons.h>
#include <netutil/icmp.h>
#include <netutil/checksum.h>
#include <netutil/ip.h>

#include <netstack/icmp.h>

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip
) {
    assert(icmp && ip);
    icmp->ip = ip;

    return SYS_ERR_OK;
}

errval_t icmp_send(
    ICMP* icmp, ip_addr_t dst_ip, uint8_t type, uint8_t code, ICMP_data field
) {
    errval_t err;
    assert(icmp);

    uint8_t *data = NULL;
    uint16_t size = 0;
    switch (type) {
    case ICMP_ER:
        ICMP_VERBOSE("Sending a reply to a ICMP echo request !");
        size = field.size;
        data = field.data;  ///ALARM: Device reserved filed for it
        assert(data);

        data -= sizeof(struct icmp_echo);
        memcpy(data, &field.echo, sizeof(struct icmp_echo));
        size += sizeof(struct icmp_echo);
        break;
    case ICMP_ECHO:
    case ICMP_DUR:
    case ICMP_SQ:
    case ICMP_RD:
    case ICMP_TE:
    case ICMP_PP:
    case ICMP_TS:
    case ICMP_TSR:
    case ICMP_IRQ:
    case ICMP_IR:
        ICMP_ERR("Not Supported ICMP type :%d!", type);
        break;
    default:
        ICMP_ERR("WRONG ICMP type :%d!", type);
        return NET_ERR_ICMP_WRONG_TYPE;
    }
    data -= sizeof(struct icmp_hdr);
    size += sizeof(struct icmp_hdr);

    struct icmp_hdr* packet = (struct icmp_hdr*) data;
    *packet = (struct icmp_hdr) {
        .type   = type,
        .code   = code,
        .chksum = 0,    /// For ICMP, the checksum is calculated for the full packet
    };
    packet->chksum = inet_checksum(packet, size);
    
    err = ip_marshal(icmp->ip, dst_ip, IP_PROTO_ICMP, (uint8_t*)packet, size);
    RETURN_ERR_PRINT(err, "Can't send the ICMP through binding");

    return SYS_ERR_OK;
}

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, uint8_t* addr, uint16_t size
) {
    errval_t err;
    assert(icmp && addr);
    struct icmp_hdr *packet = (struct icmp_hdr*) addr;

    // 1.1 Checksum
    uint16_t packet_checksum = ntohs(packet->chksum);
    packet->chksum = 0;     // Set the it as 0 to calculate
    uint16_t checksum = inet_checksum((void*)addr, size);
    if (packet_checksum != ntohs(checksum)) {
        ICMP_ERR("This ICMP Pacekt Has Wrong Checksum %p, Should be %p", checksum, packet_checksum);
        return NET_ERR_ICMP_WRONG_CHECKSUM;
    }

    addr += sizeof(struct icmp_hdr);
    size -= sizeof(struct icmp_hdr);

    uint8_t type = ICMPH_TYPE(packet);
    switch (type) {
    case ICMP_ECHO:
        addr += sizeof(struct icmp_echo);
        size -= sizeof(struct icmp_echo);
        ICMP_data field = {
            .echo = {
                .id    = ICMPH_ECHO_ID(packet),
                .seqno = ICMPH_ECHO_SEQ(packet),
            },
            .data = addr,
            .size = size,
        };
        ICMP_VERBOSE("An ICMP echo request id: %d, seqno: %d !", ntohs(field.echo.id), ntohs(field.echo.seqno));
        err = icmp_send(icmp, src_ip, ICMP_ER, 0, field);
        RETURN_ERR_PRINT(err, "Can't send reply to echo");
        break;
    case ICMP_ER:
    case ICMP_DUR:
    case ICMP_SQ:
    case ICMP_RD:
    case ICMP_TE:
    case ICMP_PP:
    case ICMP_TS:
    case ICMP_TSR:
    case ICMP_IRQ:
    case ICMP_IR:
        ICMP_ERR("Not Supported ICMP type :%d!", type);
        break;
    default:
        ICMP_ERR("WRONG ICMP type :%d!", type);
        return NET_ERR_ICMP_WRONG_TYPE;
    }

    return SYS_ERR_OK;
}
