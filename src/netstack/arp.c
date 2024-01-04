#include <netutil/etharp.h>
#include <netutil/dump.h>
#include <netutil/htons.h>
#include <netstack/ethernet.h>
#include <netstack/arp.h>

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
) {
    errval_t err = SYS_ERR_OK;
    assert(arp && ether);
    arp->ether = ether;
    arp->ip = ip;
    
    err = hash_init(
        &arp->hosts, arp->buckets, ARP_HASH_BUCKETS, HS_FAIL_ON_EXIST,
        voidptr_key_cmp, voidptr_key_hash
    );
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of ARP");

    ARP_INFO("ARP Module initialized");
    return err;
}

void arp_destroy(
    ARP* arp
) {
    assert(arp);
    hash_destroy(&arp->hosts);
    free(arp);

    ARP_NOTE("ARP module destroyed!");
}

errval_t arp_marshal(
    ARP* arp, uint16_t opration,
    ip_addr_t dst_ip, mac_addr dst_mac, Buffer buf
) {
    errval_t err;
    assert(arp);

    uint16_t send_size  = sizeof(struct arp_hdr);
    
    assert(buf.from_hdr >= ARP_HEADER_RESERVE);
    assert(buf.valid_size >= send_size);
    buf.valid_size = send_size;

    struct arp_hdr* packet = (struct arp_hdr*) buf.data;
    *packet = (struct arp_hdr) {
        .hwtype   = htons(ARP_HW_TYPE_ETH),
        .proto    = htons(ARP_PROT_IP),
        .hwlen    = ETH_ADDR_LEN,
        .protolen = ARP_PLEN_IPV4,
        .opcode   = htons(opration),
        .eth_src  = hton6(arp->ether->my_mac),
        .ip_src   = htonl(arp->ip),
        .eth_dst  = hton6(dst_mac),
        .ip_dst   = htonl(dst_ip),
    };

    err = ethernet_marshal(arp->ether, dst_mac, ETH_TYPE_ARP, buf);
    DEBUG_FAIL_RETURN(err, "Can't marshall the ARP message");
    
    return SYS_ERR_OK;
}

void arp_register(
    ARP* arp, ip_addr_t ip, mac_addr mac 
) {
    assert(arp);
    // assert(!maccmp(mac, MAC_NULL));
    // assert(!maccmp(mac, MAC_BROADCAST))

    // in 64-bit machine, a pointer is big enough to store the mac_addr, so we cast the value mac to a void pointer, not its address
    void* macaddr_as_pointer = NULL;
    memcpy(&macaddr_as_pointer, &mac, sizeof(mac_addr));

    errval_t err = hash_insert(&arp->hosts, ARP_HASH_KEY(ip), macaddr_as_pointer);
    if (err_no(err) == EVENT_HASH_EXIST_ON_INSERT) {
        ARP_INFO("The IP-MAC pair already exists");
    } else if (err_is_fail(err)) {
        DEBUG_ERR(err, "ARP can't insert this IP-MAC to the hash table !");
    }
}

errval_t arp_lookup_ip (
    ARP* arp, mac_addr mac, ip_addr_t* ret_ip
) {
    assert(arp && ret_ip);
    (void) mac;
    return SYS_ERR_NOT_IMPLEMENTED;
}

errval_t arp_lookup_mac(
    ARP* arp, ip_addr_t ip, mac_addr* ret_mac
) {
    errval_t err = SYS_ERR_OK;
    assert(arp && ret_mac);

    void* macaddr_as_pointer = NULL;
    err = hash_get_by_key(&arp->hosts, ARP_HASH_KEY(ip), &macaddr_as_pointer);
    DEBUG_FAIL_PUSH(err, NET_ERR_NO_MAC_ADDRESS, "Can't find the MAC address of given IPv4 address");

    assert(macaddr_as_pointer);
    *ret_mac = voidptr2mac(macaddr_as_pointer);

    assert(!(maccmp(*ret_mac, MAC_NULL) || maccmp(*ret_mac, MAC_BROADCAST)));

    return err;
}

errval_t arp_unmarshal(
    ARP* arp, Buffer buf
) {
    errval_t err;
    ARP_VERBOSE("Enter");
    assert(buf.valid_size == sizeof(struct arp_hdr));
    struct arp_hdr* packet = (struct arp_hdr*) buf.data;

    /// 1. Decide if the packet is correct
    if (ntohs(packet->hwtype) != ARP_HW_TYPE_ETH) {
        ARP_ERR("Hardware Type Mismatch");
        return NET_ERR_ARP_WRONG_FIELD;
    }
    if (ntohs(packet->proto) != ARP_PROT_IP) {
        ARP_ERR("Protocal Type Mismatch");
        return NET_ERR_ARP_WRONG_FIELD;
    }
    if (packet->hwlen != ETH_ADDR_LEN) {
        ARP_ERR("Hardware Length Mismatch");
        return NET_ERR_ARP_WRONG_FIELD;
    }
    if (packet->protolen != ARP_PLEN_IPV4) {
        ARP_ERR("Protocal Length Mismatch");
        return NET_ERR_ARP_WRONG_FIELD;
    }

    ip_addr_t dst_ip  = ntohl(packet->ip_dst);
    if (dst_ip != arp->ip) {
        ARP_INFO("This ARP request is for %0.8p, not us %0.8p", dst_ip, arp->ip);
        return NET_ERR_ARP_WRONG_IP_ADDRESS;
    }

    // 2. Register the IP-MAC pair
    mac_addr  src_mac = ntoh6(packet->eth_src);
    ip_addr_t src_ip  = ntohl(packet->ip_src);
    arp_register(arp, src_ip, src_mac);

    uint16_t type = ntohs(packet->opcode);
    switch (type) {
    case ARP_TYPE_REPLY:
        ARP_VERBOSE("received a ARP reply packet");
        break;
    case ARP_TYPE_REQUEST:
        ARP_VERBOSE("received a ARP request packet");
        err = arp_marshal(arp, ARP_OP_REP, src_ip, src_mac, buf);
        DEBUG_FAIL_RETURN(err, "Can't send ARP reply");
        break;
    default:
        LOG_ERR("Unknown packet type for ARP: 0x%x", type);
        return NET_ERR_ETHER_UNKNOWN_TYPE;
    }

    ARP_VERBOSE("Exit");
    return SYS_ERR_OK;
}