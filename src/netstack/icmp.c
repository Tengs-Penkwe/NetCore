#include <netutil/htons.h>
#include <netutil/icmp.h>
#include <netutil/checksum.h>
#include <netutil/ip.h>

#include <netstack/icmp.h>
#include <event/threadpool.h>
#include <event/event.h>

errval_t icmp_init(
    ICMP* icmp, struct ip_state* ip, mac_addr my_mac
) {
    errval_t err = SYS_ERR_OK;
    assert(icmp && ip);
    icmp->ip = ip;
    icmp->my_mac = my_mac;
    
    err = hash_init(
        &icmp->hosts, icmp->buckets, NDP_HASH_BUCKETS,
        HS_OVERWRITE_ON_EXIST,          // RFC 4861 requires that the address is updatable
        ipv6_key_cmp, ipv6_key_hash     // The key is ipv6_addr_t, more than 64 bits (void*), but the value can be stored in 64 bits 
    );
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of ARP");

    return err;
}

void icmp_destroy(
    ICMP* icmp
) {
    assert(icmp);

    ICMP_ERR("NYI: the hash table stores memory address, need to free them");
    hash_destroy(&icmp->hosts);

    free(icmp);
    ICMP_NOTE("ICMP module destroyed");
}

// Assumption: Caller free the buffer
// Assumption: Buf contains the data that needs to be sent
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

    const ip_context_t dst_ip_context = {
        .ipv4    = dst_ip,
        .is_ipv6 = false,
    };
    err = ip_marshal(icmp->ip, dst_ip_context, IP_PROTO_ICMP, buf);
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
    uint8_t type = packet->type;
    switch (type) {
    case ICMP_ECHO:
        struct icmp_echo* echo = (struct icmp_echo*) buf.data;
        field = (ICMP_data) {
            .echo = {
                .id    = echo->id,     // Directly from net order, no need to convert
                .seqno = echo->seqno,
            },
        };
        buffer_add_ptr(&buf, sizeof(struct icmp_echo));
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
        assert(0 && "disable for now");
        assert(err_no(err) == EVENT_ENQUEUE_FULL);
        // If the Queue if full, directly send 
        errval_t error = icmp_marshal(icmp, src_ip, ret_type, ret_code, field, buf);
        DEBUG_FAIL_RETURN(error, "After submit task failed, direct sending also failed");
        // If direct sending succeded
        return SYS_ERR_OK;
    }
    return NET_THROW_SUBMIT_EVENT;
}
