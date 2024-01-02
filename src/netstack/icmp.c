#include <netutil/htons.h>
#include <netutil/icmp.h>
#include <netutil/checksum.h>
#include <netutil/ip.h>

#include <netstack/icmp.h>
#include <event/threadpool.h>
#include <event/event.h>

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip
) {
    assert(icmp && ip);
    icmp->ip = ip;

    return SYS_ERR_OK;
}

// Assumption: Caller free the buffer
errval_t icmp_marshal(
    ICMP* icmp, ip_addr_t dst_ip, uint8_t type, uint8_t code, ICMP_data field, Buffer buf
) {
    errval_t err;
    assert(icmp);

    switch (type) {
    case ICMP_ER:
        ICMP_VERBOSE("Sending a reply to a ICMP echo request !");
        buffer_sub_ptr(&buf, sizeof(struct icmp_echo));
        memcpy(buf.data, &field.echo, sizeof(struct icmp_echo));
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
        return SYS_ERR_NOT_IMPLEMENTED;
    default:
        ICMP_ERR("WRONG ICMP type :%d!", type);
        return NET_ERR_ICMP_WRONG_TYPE;
    }
    buffer_sub_ptr(&buf, sizeof(struct icmp_hdr));

    struct icmp_hdr* packet = (struct icmp_hdr*) buf.data;
    *packet = (struct icmp_hdr) {
        .type   = type,
        .code   = code,
        .chksum = 0,    /// For ICMP, the checksum is calculated for the full packet
    };
    packet->chksum = inet_checksum_in_net_order(packet, buf.valid_size);
    
    err = ipv4_marshal(icmp->ip, dst_ip, IP_PROTO_ICMP, buf);
    DEBUG_FAIL_RETURN(err, "Can't send the ICMP through binding");
    return err;
}

errval_t icmp_unmarshal(
    ICMP* icmp, ip_addr_t src_ip, Buffer buf
) {
    errval_t err;
    assert(icmp);
    struct icmp_hdr *packet = (struct icmp_hdr*) buf.data;

    // 1.1 Checksum
    uint16_t packet_checksum = ntohs(packet->chksum);
    packet->chksum = 0;     // Set the it as 0 to calculate
    uint16_t checksum = inet_checksum_in_net_order((void*)buf.data, buf.valid_size);
    if (packet_checksum != ntohs(checksum)) {
        ICMP_ERR("This ICMP Pacekt Has Wrong Checksum %p, Should be %p", checksum, packet_checksum);
        return NET_ERR_ICMP_WRONG_CHECKSUM;
    }

    buffer_add_ptr(&buf, sizeof(struct icmp_hdr));

    // TODO: have better default value ?
    uint8_t ret_type = 0xFF;
    uint8_t ret_code = 0xFF;
    ICMP_data field;
    uint8_t type = ICMPH_TYPE(packet);
    switch (type) {
    case ICMP_ECHO:
        buffer_add_ptr(&buf, sizeof(struct icmp_echo));
        field = (ICMP_data) {
            .echo = {
                .id    = ICMPH_ECHO_ID(packet),
                .seqno = ICMPH_ECHO_SEQ(packet),
            },
        };
        ret_type = ICMP_ER;
        ret_code = 0;
        ICMP_VERBOSE("An ICMP echo request id: %d, seqno: %d !", ntohs(field.echo.id), ntohs(field.echo.seqno));
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
        return SYS_ERR_NOT_IMPLEMENTED;
    default:
        ICMP_ERR("WRONG ICMP type :%d!", type);
        return NET_ERR_ICMP_WRONG_TYPE;
    }
    assert(ret_code != 0xFF);
    assert(ret_type != 0xFF);

    ICMP_marshal* marshal = malloc(sizeof(ICMP_marshal));
    *marshal = (ICMP_marshal) {
        .icmp   = icmp,
        .dst_ip = src_ip,
        .type   = ret_type,
        .code   = ret_code,
        .field  = field,
        .buf    = buf,
    };

    err = submit_task(MK_NORM_TASK(event_icmp_marshal, (void*) marshal));
    if (err_is_fail(err))
    {
        free(marshal);
        assert(0);
        assert(err_no(err) == EVENT_ENQUEUE_FULL);
        // If the Queue if full, directly send 
        errval_t error = icmp_marshal(icmp, src_ip, ret_type, ret_code, field, buf);
        DEBUG_FAIL_RETURN(error, "After submit task failed, direct sending also failed");
        // If direct sending succeded
        return SYS_ERR_OK;
    }
    return NET_THROW_SUBMIT_EVENT;
}
