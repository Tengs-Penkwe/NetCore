#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/checksum.h>
#include <netutil/dump.h>
#include <netstack/ip.h>
#include <event/timer.h>
#include <event/threadpool.h>
#include <event/states.h>
#include <event/event.h>

#include "ip_assemble.h"
#include "ip_slice.h"

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ipv4, ipv6_addr_t my_ipv6
) {
    errval_t err = SYS_ERR_OK;
    assert(ip && ether && arp);

    ip->ether = ether;

    ip->arp       = arp;
    ip->my_ipv4   = my_ipv4;
    ip->seg_count = 0;

    ip->my_ipv6 = my_ipv6;

    ip->assembler_num  = IP_ASSEMBLER_NUM;
    // 1. Message Queue for single-thread handling of IP segmentation
    for (size_t i = 0; i < ip->assembler_num; i++)
    {
        err = assemble_init(&ip->assemblers[i], IP_ASSEMBLER_QUEUE_SIZE, i);
        if (err_is_fail(err)) {
            IP_FATAL("Can't initialize the assembler %d, TODO: free the memory", i);
            return err_push(err, SYS_ERR_INIT_FAIL);
        }
        ip->assemblers[i].ip = ip;
    }

    TCP_NOTE("IP Module Initialized, there are %d assemblers, each has %d slots",
             ip->assembler_num, IP_ASSEMBLER_QUEUE_SIZE);

    // 2. ICMP (Internet Control Message Protocol )
    ip->icmp = aligned_alloc(ATOMIC_ISOLATION, sizeof(ICMP)); 
    assert(ip->icmp); memset(ip->icmp, 0x00, sizeof(ICMP));
    err = icmp_init(ip->icmp, ip, ether->my_mac);
    DEBUG_FAIL_RETURN(err, "Can't initialize global ICMP state");

    // 3. UDP (User Datagram Protocol)
    ip->udp = aligned_alloc(ATOMIC_ISOLATION, sizeof(UDP)); 
    assert(ip->udp); memset(ip->udp, 0x00, sizeof(UDP));
    err = udp_init(ip->udp, ip);
    DEBUG_FAIL_RETURN(err, "Can't initialize global UDP state");

    // 4. TCP (Transmission Control Protocol)
    ip->tcp = aligned_alloc(ATOMIC_ISOLATION, sizeof(TCP));
    assert(ip->tcp); memset(ip->tcp, 0x00, sizeof(TCP));
    err = tcp_init(ip->tcp, ip);
    DEBUG_FAIL_RETURN(err, "Can't initialize global TCP state");

    return err;

    //TODO: have better error handling (resource release)
}

void ip_destroy(
    IP* ip
) {
    assert(ip);

    for (size_t i = 0; i < ip->assembler_num; i++)
    {
        assembler_destroy(&ip->assemblers[i], i);
    }
    LOG_ERR("ICMP, UDP, TCP, they need to be destroyed");

    icmp_destroy(ip->icmp);
    
    free(ip);
}


errval_t handle_ip_segment_assembly(
    IP* ip, ip_addr_t src_ip, uint8_t proto, uint16_t id, Buffer buf, uint16_t offset, bool more_frag, bool no_frag
) {
    errval_t err = SYS_ERR_OK; assert(ip);
    IP_INFO("Assembling a message, ID: %d, size: %d, offset: %d, no_frag: %d, more_frag: %d", id, buf.valid_size, offset, no_frag, more_frag);

    // 1. if no_frag, then we can directly handle it (how simple it is! compared to the segmented one, which requires significant amount of work)
    if (no_frag == true ||
       (more_frag == false && offset == 0)) {  // Which means this isn't a segmented packet
                                                   
        assert(offset == 0 && more_frag == false);
        
        err = ipv4_handle(ip, proto, src_ip, buf);
        DEBUG_FAIL_RETURN(err, "Can't handle this IP message ?");
        return err;
    }

    // 2. if the message is segmented, then we need to put it into the assembler
    ip_msg_key_t key = ip_message_hash(src_ip, id);

    // 2.1 Create the message structure
    IP_segment *msg = calloc(1, sizeof(IP_segment)); assert(msg);
    *msg = (IP_segment) {
        .assembler  = &ip->assemblers[key],
        .src_ip    = src_ip,
        .proto     = proto,
        .id        = id,
        .offset    = offset,
        .more_frag = more_frag,
        .no_frag   = no_frag,
        .buf       = buf,
    };

    // 3. Add the message to the assembler's queue, we do this to ensure the message is handled in a single thread. To handle the 
    //   segmentation in multi-thread is too complicated, requires a lot of synchronization, and it's rarely used, doesn't worth it
    Task task_for_assembler = MK_TASK(&ip->assemblers[key].event_que, &ip->assemblers[key].event_come, event_ip_assemble, (void*)msg);
    err = submit_task(task_for_assembler);
    if (err_is_fail(err)) {
        assert(err_no(err) == EVENT_ENQUEUE_FULL);
        // Will be freed in upper module (event caller)
        // free_buffer(buf);
        free(msg);
        IP_WARN("Too much IP segmentation message for bucket %d, will drop it in upper module", key);
        return err;
    } else {
        return err_push(err, NET_THROW_SUBMIT_EVENT);
    }
}

errval_t ipv4_unmarshal(
    IP* ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(ip);
    struct ip_hdr* packet = (struct ip_hdr*)buf.data;
    
    /// 1. Service Type
    if (packet->tos != 0x00) {
        IP_ERR("We Don't Support TOS Field: %p, But I'll Ignore it for Now", packet->tos);
        // return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 1.1 Header Size
    const uint16_t header_size = IPH_HL(packet);
    if (header_size != sizeof(struct ip_hdr)) {
        IP_NOTE("The IP Header has %d Bytes, We don't have special treatment for it", header_size);
    }
    if (!(header_size >= IPH_LEN_MIN && header_size <= IPH_LEN_MAX)) {
        IP_ERR("IPv4 Header to Big or Small: %d", header_size);
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 1.2 Packet Size check
    if (ntohs(packet->total_len) != buf.valid_size) {
        LOG_ERR("IP Packet Size Unmatch %p v.s. %p", ntohs(packet->total_len), buf.valid_size);
        return NET_ERR_IPv4_WRONG_FIELD;
    }
    if (buf.valid_size < IP_LEN_MIN) {
        LOG_ERR("IPv4 Packet too Small: %d", buf.valid_size);
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 1.3 Checksum
    uint16_t packet_checksum = ntohs(packet->chksum);
    packet->chksum = 0;     // Set the it as 0 to calculate
    uint16_t checksum = inet_checksum_in_net_order(packet, header_size);
    if (packet_checksum != ntohs(checksum)) {
        LOG_ERR("This IPv4 Pacekt Has Wrong Checksum %p, Should be %p", checksum, packet_checksum);
        return NET_ERR_IPv4_WRONG_CHECKSUM;
    }

    // 1.4 Destination IP
    ip_addr_t dst_ip = ntohl(packet->dest);
    if (dst_ip != ip->my_ipv4) {
        LOG_ERR("This IPv4 Pacekt isn't for us %0.8X but for %0.8X", ip->my_ipv4, dst_ip);
        return NET_ERR_IPv4_WRONG_IP_ADDRESS;
    }

    // TODO: Cache the IP & MAC in ARP
    // Re-consider it, this may break the layer model ?

    // 2. Fragmentation
    const uint16_t id             = ntohs(packet->id);
    const uint16_t flag_offset    = ntohs(packet->offset);
    const bool     flag_reserved  = flag_offset & IP_RF;
    const bool     flag_no_frag   = flag_offset & IP_DF;
    const bool     flag_more_frag = flag_offset & IP_MF;
    const uint16_t offset         = (flag_offset & IP_OFFMASK) * 8;
    if (flag_reserved || (flag_no_frag && flag_more_frag)) {
        LOG_ERR("Problem with flags, reserved: %d, no_frag: %d, more_frag: %d", flag_reserved, flag_no_frag, flag_more_frag);
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 2.1 Find the corresponding MAC address
    ip_addr_t src_ip = ntohl(packet->src);
    mac_addr src_mac = MAC_NULL;
    err = arp_lookup_mac(ip->arp, src_ip, &src_mac);
    if (err_no(err) == NET_ERR_NO_MAC_ADDRESS) {
        USER_PANIC_ERR(err, "You received an IP message, but you don't know the IP-MAC pair ?");
    } else
        DEBUG_FAIL_RETURN(err, "Can't find binding for given IP address");

    // 2.2 Remove the IP header
    buffer_add_ptr(&buf, header_size);

    // 3. Assemble the IP message
    uint8_t proto = packet->proto;
    err = handle_ip_segment_assembly(ip, src_ip, proto, id, buf, offset, flag_more_frag, flag_no_frag);
    DEBUG_FAIL_RETURN(err, "Can't assemble the IP message from the packet");

    // 3.1 TTL: TODO, should we deal with it ?
    (void) packet->ttl;

    return err;
}

errval_t ip_marshal(    
    IP* ip, ip_context_t dst_ip, uint8_t proto, Buffer buf
) {
    assert(ip); errval_t err = SYS_ERR_OK;
    IP_send *msg = malloc(sizeof(IP_send)); assert(msg);

    // 1. Create the message
    *msg = (IP_send) {
        .ip             = ip,
        .dst_ip         = dst_ip,
        .proto          = proto,
        .id             = dst_ip.is_ipv6 ? 0 : (uint16_t)atomic_fetch_add(&ip->seg_count, 1),
        .sent_size      = 0,    // IPv4 only
        .dst_mac        = MAC_NULL,
        .buf            = buf,
        .retry_interval = IP_RETRY_SEND_US,
    };

    // 3. Get destination MAC
    err = lookup_mac(ip, dst_ip, &msg->dst_mac);
    switch (err_no(err))
    {
    case NET_ERR_NO_MAC_ADDRESS:   // Get Address first
    {
        msg->retry_interval = GET_MAC_WAIT_US;
        submit_delayed_task(MK_DELAY_TASK(msg->retry_interval, close_sending_message, MK_NORM_TASK(check_get_mac, (void*)msg)));
        return NET_THROW_SUBMIT_EVENT;
    }
    case SYS_ERR_OK: { // Continue sending
        // TODO: check multicast
        assert(!(maccmp(msg->dst_mac, MAC_NULL) || maccmp(msg->dst_mac, MAC_BROADCAST)));
        check_send_message((void*)msg);
        return NET_THROW_SUBMIT_EVENT;
    }
    default: USER_PANIC_ERR(err, "Can't establish binding for given IP address");
        return err;
    }
}