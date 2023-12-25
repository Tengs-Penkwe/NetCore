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
    size_t size = 0;
    switch (type) {
    case ICMP_ER:
        ICMP_VERBOSE("Sending a reply to a ICMP echo request !");
        size = sizeof(struct icmp_hdr) + sizeof(struct icmp_echo) + field.size;
        data = (uint8_t*)malloc(size);
        assert(data);
        memcpy(data + sizeof(struct icmp_hdr), &field, sizeof(struct icmp_echo));
        memcpy(data + sizeof(struct icmp_hdr) + sizeof(struct icmp_echo), field.data, field.size);
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

    struct icmp_hdr* packet = (struct icmp_hdr*) data;
    packet->type = type;
    packet->code = code;

    /// For ICMP, the checksum is calculated for the full packet
    packet->chksum = 0;
    const uint16_t checksum = inet_checksum(packet, size);
    packet->chksum = checksum;
    
    err = ip_marshal(icmp->ip, dst_ip, IP_PROTO_ICMP, (uint8_t*)packet, size);
    RETURN_ERR_PRINT(err, "Can't send the ICMP through binding");

    return SYS_ERR_OK;
}

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, uint8_t* addr, size_t size
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
        ICMP_data data = {
            .echo = {
                .id    = ICMPH_ECHO_ID(packet),
                .seqno = ICMPH_ECHO_SEQ(packet),
            },
            .data = addr,
            .size = size,
        };
        ICMP_VERBOSE("An ICMP echo request id: %d, seqno: %d !", ntohs(data.echo.id), ntohs(data.echo.seqno));
        err = icmp_send(icmp, src_ip, ICMP_ER, 0, data);
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
