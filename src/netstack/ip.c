#include <netutil/ip.h>
#include <netstack/ip.h>
static errval_t ip_handle(IP_message* msg);

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
    errval_t err;
    assert(ip && ether && arp);

    ip->ip = my_ip;
    ip->ether = ether;
    ip->arp = arp;

    ip->seg_count = 0;

    /// Initialize the messages
    if (cc_hashtable_new(&ip->recv_messages) != CC_OK) {
        IP_ERR("Can't initialize the hashtable for receiving message");
        return SYS_ERR_FAIL;
    }
    if (cc_hashtable_new(&ip->send_messages) != CC_OK) {
        IP_ERR("Can't initialize the hashtable for sending message");
        return SYS_ERR_FAIL;
    }

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
    err = ip_assemble(ip, src_ip, proto, identification, data, size, offset, flag_more_frag);
    RETURN_ERR_PRINT(err, "Can't assemble the IP message from the packet");

    // 3.1 TTL: TODO, should we deal with it ?
    (void) packet->ttl;

    return SYS_ERR_OK;
}