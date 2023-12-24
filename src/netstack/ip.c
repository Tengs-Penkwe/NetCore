#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/checksum.h>
#include <netutil/dump.h>
#include <netstack/ip.h>
#include <event/timer.h>
#include <event/threadpool.h>

// Forward declaration
static void check_get_mac(void* message);
static void check_send_message(void* message);
static errval_t ip_handle(IP_message* msg);

typedef struct {
    void*  data;
    size_t size;
    size_t offset;
} Mseg;

/**
 * @brief Compare if a segmentation is duplicate
 */
static inline int seg_same(const void* seg, const void* target) {
    if (((Mseg *)seg)->offset == ((Mseg *)target)->offset &&
        ((Mseg *)seg)->size   == ((Mseg *)target)->size) {
        return 0;   // Same segmentation
    } else  {
        return -1;  // Different segmentation
    }
}

/**
 * @brief Used with cc_array_reduce, get the whole size
 */
static inline void seg_size(void* pre, void* next, void* result) {
    size_t* recvd_size = result;
    *recvd_size = ((Mseg*)pre)->size + ((Mseg*)next)->size;
}

///TODO: assert the offset + size == next ?
static inline int seg_cmp(const void* pre, const void* next) {
    return (((Mseg*)pre)->offset  < ((Mseg*)next)->offset) -
           (((Mseg*)next)->offset < ((Mseg*)pre)->offset) ;
}

static inline void seg_collect(void* segment) {
    Mseg* seg = segment;
}

/**
 * @brief Frees resources associated with an IP message: to be used in cc array
 */
static inline void free_segs(void *segment) {
    Mseg* seg = segment;
    free(seg->data);
    free(segment);
    segment = NULL;
}

static inline void free_message(void* message) {
    IP_message* msg = message;
    cc_array_destroy_cb(msg->segs, free_segs);
    pthread_mutex_destroy(&msg->mutex);
    free(message);
    message = NULL;
}

static errval_t mac_lookup(
    IP* ip, ip_addr_t dst_ip, mac_addr* dst_mac
) {
    errval_t err;
    assert(ip);

    err = arp_lookup_mac(ip->arp, dst_ip, dst_mac);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't get the corresponding MAC address of IP address");
        errval_t error = arp_send(ip->arp, ARP_OP_REQ, dst_ip, MAC_BROADCAST);
        DEBUG_ERR(error, "I Can't even send an ARP request after I can't find the MAC for given IP");
        return err_push(err, NET_ERR_IPv4_NO_MAC_ADDRESS);
    }

    assert(!(maccmp(*dst_mac, MAC_NULL) || maccmp(*dst_mac, MAC_BROADCAST)));

    return SYS_ERR_OK;
}

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ip
) {
    // errval_t err;
    assert(ip && ether && arp);

    ip->ip = my_ip;
    ip->ether = ether;
    ip->arp = arp;

    ip->seg_count = 0;
    if (pthread_mutex_init(&ip->recv_mutex, NULL) != 0) {
        ARP_ERR("Can't initialize the mutex for IP");
        return SYS_ERR_FAIL;
    }
    
    ip->recv_messages = kh_init(ip_msg);
    ip->send_messages = kh_init(ip_msg);

    // ip->icmp = calloc(1, sizeof(ICMP));
    // assert(ip->icmp);
    // err = icmp_init(ip->icmp, ip);
    // RETURN_ERR_PRINT(err, "Can't initialize global ICMP state");

    // ip->udp = calloc(1, sizeof(UDP));
    // assert(ip->udp);
    // err = udp_init(ip->udp, ip);
    // RETURN_ERR_PRINT(err, "Can't initialize global UDP state");

    // ip->tcp = calloc(1, sizeof(TCP));
    // assert(ip->tcp);
    // err = tcp_init(ip->tcp, ip);
    // RETURN_ERR_PRINT(err, "Can't initialize global TCP state");

    return SYS_ERR_OK;
}

/**
 * @brief Checks the status of an IP message, drops it if TTL expired, or processes it if complete.
 *        This functions is called in a periodic event
 * 
 * @param message Pointer to the IP_message structure to be checked.
 */
static void check_recvd_message(void* message) {
    IP_VERBOSE("Checking a message");
    errval_t err;
    IP_message* msg = message;
    IP* ip = msg->ip;
    assert(msg && ip);
    ip_msg_key_t msg_key = MSG_KEY(msg->recvd.src_ip, msg->id);

    /// Also modified in ip_assemble(), be careful of global states
    msg->recvd.times_to_live *= 1.5;
    if (msg->recvd.times_to_live >= IP_GIVEUP_RECV_US) goto close_message;

    if (msg->whole_size != SIZE_DONT_KNOW) { // At least we have the last segmentation, try to assemble all segmentations
        // We don't need to care about duplicate segment here, they are deal in ip_assemble



        size_t recvd_size = 0;


        // Get the received size
        pthread_mutex_lock(&msg->mutex);
        cc_array_reduce(msg->segs, seg_size, &recvd_size);
        pthread_mutex_unlock(&msg->mutex);

        // We collected all the segments ! remove it first
        if (recvd_size == msg->whole_size) {
            ///TODO: lock free
            pthread_mutex_lock(&ip->recv_mutex);
            kh_del(ip_msg, ip->recv_messages, msg_key);
            pthread_mutex_unlock(&ip->recv_mutex);

            ///TODO: Other thread may still hold this lock !
            cc_array_sort(msg->segs, seg_cmp);
        

        }
    }

    err = ip_handle(msg);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "We meet an error when handling an IP message");
    }

    submit_delayed_task(msg->recvd.times_to_live, MK_TASK(check_recvd_message, (void*)msg));

    IP_VERBOSE("Done Checking a message, ttl: %d, whole size: %p", msg->recvd.times_to_live, msg->whole_size);
    return;

close_message:
    ip_msg_key_t msg_key = MSG_KEY(msg->recvd.src_ip, msg->id);
    pthread_mutex_lock(&ip->recv_mutex);
    kh_del(ip_msg, ip->recv_messages, msg_key);
    pthread_mutex_unlock(&ip->recv_mutex);
    IP_NOTE("Closing a message");
}

/**
 * @brief Assemble an IP packet to message
 *
 * @param[in]   bind        IP_binding struct stores all the states
 * @param[in]   id          Identification number of the message.
 * @param[in]   msg_len     Length of the whole message
 * @param[in]   data        The start of the bufffer to copy from
 * @param[in]   size        Length of this packet
 *
 * @return Returns error code indicating success or failure.
 */
static errval_t ip_assemble(
    IP* ip, ip_addr_t src_ip, uint8_t proto, uint16_t id, uint8_t* addr, size_t size, uint32_t offset, bool more_frag, bool no_frag
) {
    errval_t err;
    assert(ip && addr);
    IP_DEBUG("Assembling a message, ID: %d, addr: %p, size: %d, offset: %d, more_frag: %d", id, addr, size, offset, more_frag);

    ip_msg_key_t msg_key = MSG_KEY(src_ip, id);
    IP_message* msg = NULL;

    ///TODO: add lock ?
    khint64_t key = kh_get(ip_msg, ip->recv_messages, msg_key);
    // Try to find if it already exists
    if (key == kh_end(ip->recv_messages)) {  // This message doesn't exist yet
        msg = calloc(1, sizeof(IP_message)); assert(msg);
        *msg = (IP_message) {
            .ip         = ip,
            .proto      = proto,
            .id         = id,
            .whole_size = SIZE_DONT_KNOW,     // We don't know the size util the last packet arrives
            .segs       = NULL,
            .data       = NULL,
            .mutex      = { 0 },
            .recvd      = {
                .times_to_live = IP_RETRY_RECV_US,
                .src_ip = src_ip,
            },
        };

        ///TODO: What if 2 threads received duplicate segmentation at the same time ?
        if ((more_frag == 0 && offset == 0) || no_frag == 0) { // Which means this isn't a segmented packet
            assert(offset == 0 && more_frag == 0);

            msg->data = addr;
            assert(msg->whole_size == SIZE_DONT_KNOW);
            msg->whole_size = size;
            check_recvd_message((void*) msg);

            return SYS_ERR_OK;
        }

        if (cc_array_new(&msg->segs) != CC_OK) {
            IP_ERR("Can't allocate a new array for IP message");
            return SYS_ERR_ALLOC_FAIL;
        }

        Mseg* seg = calloc(1, sizeof(Mseg)); assert(seg);
        *seg = (Mseg) {
            .data   = addr,
            .size   = size,
            .offset = offset,
        };

        if (cc_array_add(msg->segs, seg) != CC_OK) {
            IP_ERR("Can't add the segmentation to IP message");
            return SYS_ERR_ALLOC_FAIL;
        }

        ///TODO: Should I lock it before kh_get, if thread A is changing the hash table while thread
        // B is reading it, will it cause problem ? 
        pthread_mutex_lock(&ip->recv_mutex);
        int ret;
        key = kh_put(ip_msg, ip->recv_messages, msg_key, &ret); 
        if (!ret) { // Can't put key into hash table
            kh_del(ip_msg, ip->recv_messages, key);
            USER_PANIC("Can't add a new message with seqno: %d to hash table", id);
            print_ip_address(src_ip);
            dump_ipv4_header(addr);
        }
        // Set the value of key
        kh_value(ip->recv_messages, key) = msg;
        pthread_mutex_unlock(&ip->recv_mutex);

        // Multiple threads may handle different segmentation of IP at same time, need to deal with it 
        if (pthread_mutex_init(&msg->mutex, NULL) != 0) {
            IP_ERR("Can't initialize the mutex for IP message");
            return SYS_ERR_INIT_FAIL;
        }
        
    } else {
        // TODO: should I accquire the lock ?
        msg = kh_get(ip_msg, ip->recv_messages, msg_key); assert(msg);
        assert(msg->proto == proto);
        ///ALARM: global state, also modified in check_recvd_message
        msg->recvd.times_to_live /= 1.5;  // We got 1 more packet, wait less time

        Mseg* seg = calloc(1, sizeof(Mseg)); assert(seg);
        *seg = (Mseg) {
            .data   = addr,
            .size   = size,
            .offset = offset,
        };

        if (cc_array_contains(msg->segs, seg, seg_same) != 0) {  /// Duplicate segmentations
            IP_ERR("We have duplicate IP message segmentation");
            free(seg);
            return NET_ERR_IPv4_DUPLITCATE_SEG;
        }
        pthread_mutex_lock(&msg->mutex);
        assert(cc_array_add(msg->segs, seg) == CC_OK);
        pthread_mutex_unlock(&msg->mutex);
    }

    // If this the laset packet, we now know the final size
    if (more_frag == 0) {
        assert(no_frag == false);

        assert(msg->whole_size == SIZE_DONT_KNOW);
        msg->whole_size = offset + size;
    }

    // We need to receive other segmentation of this message
    submit_delayed_task(msg->recvd.times_to_live, MK_TASK(check_recvd_message, (void*)msg));
    
    return SYS_ERR_OK;
}

errval_t ip_unmarshal(
    IP* ip, uint8_t* data, size_t size
) {
    assert(ip && data);
    errval_t err;
    struct ip_hdr* packet = (struct ip_hdr*)data;
    
    /// 1. Decide if the packet is correct
    if (IPH_V(packet) != 4) {
        LOG_ERR("IP Protocal Version Mismatch");
        return NET_ERR_IPv4_WRONG_FIELD;
    }
    if (packet->tos != 0x00) {
        LOG_ERR("We Don't Support TOS Field: %p, But I'll Ignore it for Now", packet->tos);
        // return NET_ERR_IPv4_WRONG_FIELD;
    }
    if (IPH_HL(packet) != sizeof(struct ip_hdr)) {
        LOG_ERR("We Only Support IP Header Length as 20 Bytes");
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 1.2 Packet Size check
    if (ntohs(packet->len) != size) {
        LOG_ERR("IP Packet Size Unmatch %p v.s. %p", ntohs(packet->len), size);
        return NET_ERR_IPv4_WRONG_FIELD;
    }
    if (!(size > IP_LEN_MIN && size < IP_LEN_MAX)) {
        LOG_ERR("IPv4 Packet to Big or Small: %d", size);
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 1.3 Checksum
    uint16_t packet_checksum = ntohs(packet->chksum);
    packet->chksum = 0;     // Set the it as 0 to calculate
    uint16_t checksum = inet_checksum(packet, sizeof(struct ip_hdr));
    if (packet_checksum != ntohs(checksum)) {
        LOG_ERR("This IPv4 Pacekt Has Wrong Checksum %p, Should be %p", checksum, packet_checksum);
        return NET_ERR_IPv4_WRONG_CHECKSUM;
    }

    // 1.4 Destination IP
    ip_addr_t dst_ip = ntohl(packet->dest);
    if (dst_ip != ip->ip) {
        LOG_ERR("This IPv4 Pacekt isn't for us %p but for %p", ip->ip, dst_ip);
        return NET_ERR_IPv4_WRONG_IP_ADDRESS;
    }

    // TODO: Cache the IP & MAC in ARP
    // Re-consider it, this may break the layer model ?

    // 2. Fragmentation
    const uint16_t identification = ntohs(packet->id);
    const uint16_t flag_offset = ntohs(packet->offset);
    const bool flag_reserved = flag_offset & IP_RF;
    const bool flag_no_frag  = flag_offset & IP_DF;
    const bool flag_more_frag= flag_offset & IP_MF;
    const uint32_t offset    = (flag_offset & IP_OFFMASK) * 8;
    assert(offset <= 0xFFFF);
    if (flag_reserved || (flag_no_frag && flag_more_frag)) {
        LOG_ERR("Problem with flags, reserved: %d, no_frag: %d, more_frag: %d", flag_reserved, flag_no_frag, flag_more_frag);
        return NET_ERR_IPv4_WRONG_FIELD;
    }

    // 2.1 Find or Create the binding
    ip_addr_t src_ip = ntohl(packet->src);
    mac_addr src_mac = MAC_NULL;
    err = mac_lookup(ip, src_ip, &src_mac);
    if (err_no(err) == NET_ERR_IPv4_NO_MAC_ADDRESS) {
        USER_PANIC_ERR(err, "You received a message, but you don't know the IP-MAC pair ?");
    } else
        RETURN_ERR_PRINT(err, "Can't find binding for given IP address");

    data += sizeof(struct ip_hdr);
    size -= sizeof(struct ip_hdr);

    // 3. Assemble the IP message
    uint8_t proto = packet->proto;
    // err = ip_assemble(ip, src_ip, proto, identification, data, size, offset, flag_more_frag, flag_no_frag);
    // RETURN_ERR_PRINT(err, "Can't assemble the IP message from the packet");
    (void) identification;
    (void) proto;
    (void) err;
    IP_ERR("HEE");

    // 3.1 TTL: TODO, should we deal with it ?
    (void) packet->ttl;

    return SYS_ERR_OK;
}

/**
 * @brief Unmarshals a complete IP message and processes it based on its protocol type.
 * 
 * @param msg Pointer to the IP message to be unmarshalled.
 * 
 * @return Returns error code indicating success or failure.
 */
__attribute_maybe_unused__
static errval_t ip_handle(IP_message* msg) {
    // errval_t err;
    IP_VERBOSE("An IP Message has been assemble, now let's process it");

    switch (msg->proto) {
    case IP_PROTO_ICMP:
        IP_VERBOSE("Received a ICMP packet");
        // err = icmp_unmarshal(msg->ip->icmp, msg->recvd.src_ip, msg->, msg->recvd.size);
        // RETURN_ERR_PRINT(err, "Error when unmarshalling an ICMP message");
        break;
    case IP_PROTO_UDP:
        IP_VERBOSE("Received a UDP packet");
        // err = udp_unmarshal(msg->ip->udp, msg->recvd.src_ip, msg->, msg->recvd.size);
        // RETURN_ERR_PRINT(err, "Error when unmarshalling an UDP message");
        break;
    case IP_PROTO_IGMP:
    case IP_PROTO_UDPLITE:
        LOG_ERR("Unsupported Protocal");
        return SYS_ERR_NOT_IMPLEMENTED;
    case IP_PROTO_TCP:
        IP_VERBOSE("Received a TCP packet");
        // err = tcp_unmarshal(msg->ip->tcp, msg->recvd.src_ip, msg->, msg->recvd.size);
        // RETURN_ERR_PRINT(err, "Error when unmarshalling an TCP message");
        break;
    default:
        LOG_ERR("Unknown packet type for IPv4: %p", msg->proto);
        return NET_ERR_IPv4_WRONG_PROTOCOL;
    }

    IP_VERBOSE("Done unmarshalling a IPv4 message");
    return SYS_ERR_OK;
}