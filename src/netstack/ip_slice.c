#include "ip_slice.h"

// static void check_get_mac(void* message) {
//     LOG_TRIVIAL("check bind");
//     errval_t err;
//     IP_message* msg = message;
//     IP* ip = msg->ip;
//     assert(msg && ip);

//     mac_addr dst_mac = MAC_NULL;
//     err = mac_lookup(ip, msg->sent.dst_ip, &dst_mac);
//     if (err_no(err) == NET_ERR_IPv4_NO_MAC_ADDRESS) {

//         LOG_INFO("Can't find the Corresponding IP address, sent request, retry later in %d ms", msg->sent.retry_interval);
//         msg->sent.retry_interval *= 2;
//         if (msg->sent.retry_interval >= IP_GIVEUP_SEND_US) goto close_message; 

//         err = deferred_event_register(&msg->defer, ip->ws, msg->sent.retry_interval, MKCLOSURE(check_get_mac, (void*)msg));
//         if (err_is_fail(err)) {
//             free_message(msg);
//             USER_PANIC_ERR(err, "We can't register a deferred event to deal with the IP message"
//                                 "TODO: let's deal with it later");
//         }
//     } else if (err_is_fail(err)) {

//         DEBUG_ERR(err, "Can't handle this binding error in a closure, but the process continue");
//         goto close_message;
//     } else {
//         assert(maccmp(msg->sent.dst_mac, MAC_NULL));
//         msg->sent.dst_mac = dst_mac;
        
//         // Begin sending message
//         msg->sent.retry_interval = IP_RETRY_SEND_US;
//         assert(msg->id == 0);  // It should be the first message in this binding ?     
//         err = deferred_event_register(&msg->defer, ip->ws, msg->sent.retry_interval, MKCLOSURE(check_send_message, (void*)msg));
//         if (err_is_fail(err)) {
//             free_message(msg);
//             USER_PANIC_ERR(err, "We can't register a deferred event to deal with the IP message"
//                                 "TODO: let's deal with it later");
//         }
//     }
//     LOG_TRIVIAL("Exit check bind");
//     return;

// close_message:
//     free_message((void*)msg);
//     LOG_ERR("Can't get the ARP done, close sending a message"); 
// }

// static void check_send_message(void* message) {
//     LOG_TRIVIAL("Check sending a message");
//     errval_t err;
//     IP_message* msg = message;
//     IP* ip = msg->ip;
//     assert(msg && ip);

//     if (msg->sent.retry_interval > IP_GIVEUP_SEND_US) goto close_message;

//     err = ip_send(msg);
//     if (err_is_fail(err)) {
//         DEBUG_ERR(err, "We failed sending an IP packet, but we won't give up, we will try another in %d milliseconds !", msg->sent.retry_interval / 1000);
//         msg->sent.retry_interval *= 2;
//     } 
    
//     if (msg->sent.size == msg->whole_size) { // We are done sending !
//         LOG_DEBUG("We done sending an IP message ! size: %d, retry interval in ms: %d", msg->whole_size, msg->sent.retry_interval / 1000);
//         goto close_message;
//     }

//     err = deferred_event_register(&msg->defer, ip->ws, msg->sent.retry_interval, MKCLOSURE(check_send_message, (void*)msg));
//     if (err_is_fail(err)) {
//         DEBUG_ERR(err, "We can't re-register ourselves sending message");
//         goto close_message;
//     }

//     LOG_TRIVIAL("Done Checking a sending message, ttl: %d us, whole size: %d, snet size: %d", msg->sent.retry_interval / 1000, msg->whole_size, msg->sent.size);
//     return;

// close_message:
//     uint64_t msg_key = MSG_KEY(msg->sent.dst_ip, msg->id);
//     collections_hash_delete(ip->send_messages, msg_key);
//     LOG_ERR("Close sending a message"); 
// }


/// Assumption: sending is serial, there is only 1 thread sending a slice at one time
///             NO CONCURRENT sending, different slices maybe sent by different thread, but there is no contention
/// @return error code, depends on user if he/she want to retry
errval_t ip_send(
    IP* ip, const ip_addr_t dst_ip, const mac_addr dst_mac,
    const uint16_t id, const uint8_t proto, const uint8_t *data, 
    const uint16_t send_from, const uint16_t size_to_send, bool last_slice
) {
    errval_t err; assert(ip && data);
    
    // 1. Calculate the information of segmentation
    assert(send_from >= 0 && send_from % 8 == 0);
    uint16_t offset = send_from / 8;
    uint16_t flag_offset = offset & IP_OFFMASK;

    // Reserved Field should be 0
    OFFSET_RF_SET(flag_offset, 0);

    uint16_t pkt_size = size_to_send + sizeof(struct ip_hdr);
    // if the packet is less than 576 byte, we set the non-fragment flag
    bool no_frag = (pkt_size <= IP_MINIMUM_NO_FRAG) && (offset == 0);
    OFFSET_DF_SET(flag_offset, no_frag);

    // More Fragment Field should be 0 for last fragementation
    if (last_slice) OFFSET_MF_SET(flag_offset, 0);    
    else            OFFSET_MF_SET(flag_offset, 1);
    
    // 2. Prepare the send buffer
    /// ALARM: should have reserved space before !
    uint8_t* data_to_send = (uint8_t*)data;
    data_to_send -= sizeof(struct ip_hdr);
    /// ALARM: This will destroy the segement before, but since we have sent them, it's ok

    // 3. Fill the header
    struct ip_hdr *packet = (struct ip_hdr*) data_to_send;
    *packet = (struct ip_hdr) {
        .ihl     = 0x5,
        .version = 0x4,
        .tos     = 0x00,
        .len     = htons(pkt_size),
        .id      = htons(id),
        .offset  = htons(flag_offset),
        .ttl     = 0xFF,
        .proto   = proto,
        .chksum  = 0,
        .src     = htonl(ip->ip),
        .dest    = htonl(dst_ip),
    };
    packet->chksum = inet_checksum(packet, sizeof(struct ip_hdr));

    // 4. Send the packet
    err = ethernet_marshal(ip->ether, dst_mac, ETH_TYPE_IPv4, (uint8_t*)packet, pkt_size);
    RETURN_ERR_PRINT(err, "Can't send the IPv4 packet");

    IP_VERBOSE("End sending an IP packet");
    return SYS_ERR_OK;
}

errval_t ip_slice(IP_send* msg) {
    errval_t err;       assert(msg);
    IP* ip = msg->ip;   assert(ip);

    IP_VERBOSE("Sending IP Message: Protocal %x, data: %p, whole size: %d, sent size: %d, retry in %d ms",
                msg->proto, msg->data, msg->whole_size, msg->sent_size, msg->retry_interval / 1000);

    // 1. Get the destination MAC, IP and message ID
    const uint16_t  whole_size = msg->whole_size;
    const uint16_t  sent_size  = msg->sent_size;
    const uint8_t  *data       = msg->data;
    assert(sent_size < whole_size);
    assert(whole_size <= 0xFFFF);  // 65,535
    assert(sent_size >= 0 && sent_size % 8 == 0);

    // 2. Marshal and send sliceS
    for (int size_left = (int)(whole_size - sent_size); size_left > 0; size_left -= IP_MTU) {

        // 2.1 Calculate packet size
        bool last_slice         = (size_left < (int)IP_MTU);
        const uint16_t seg_size = last_slice ? (uint16_t)size_left : IP_MTU;

        err = ip_send(ip, msg->dst_ip, msg->dst_mac, msg->id, msg->proto,
            data, sent_size, seg_size, last_slice);
        if (err_is_fail(err)) {
            IP_INFO("Sending a segment failed, will try latter in %d ms", msg->retry_interval / 1000);
            return err;
        }
        // ALARM: single thread sending
        msg->sent_size += seg_size;
    }

    return SYS_ERR_OK;
}