#include <netutil/etharp.h>
#include <netutil/dump.h>
#include <netutil/htons.h>
#include <netstack/ethernet.h>
#include <netstack/arp.h>

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
) {
    assert(arp && ether);
    arp->ether = ether;
    arp->ip = ip;
    arp->hosts = kh_init(ip_mac); 

    if (pthread_mutex_init(&arp->mutex, NULL) != 0) {
        ARP_ERR("Can't initialize the mutex for ARP");
        return SYS_ERR_FAIL;
    }

    return SYS_ERR_OK;
}

errval_t arp_send(
    ARP* arp, uint16_t opration,
    ip_addr_t dst_ip, mac_addr dst_mac
) {
    errval_t err;
    assert(arp);

    size_t send_size = ARP_RESERVE_SIZE + sizeof(struct arp_hdr);
    uint8_t* data_with_reserve = malloc(send_size);
    assert(data_with_reserve);

    struct arp_hdr* packet = (struct arp_hdr*) data_with_reserve + ARP_RESERVE_SIZE;
    *packet = (struct arp_hdr) {
        .hwtype   = htons(ARP_HW_TYPE_ETH),
        .proto    = htons(ARP_PROT_IP),
        .hwlen    = ETH_ADDR_LEN,
        .protolen = ARP_PLEN_IPV4,
        .opcode   = htons(opration),
        .eth_src  = hton6(arp->ether->mac),
        .ip_src   = htonl(arp->ip),
        .eth_dst  = hton6(dst_mac),
        .ip_dst   = htonl(dst_ip),
    };

    err = ethernet_marshal(arp->ether, dst_mac, ETH_TYPE_ARP, (uint8_t*)packet, send_size);
    RETURN_ERR_PRINT(err, "Can't marshall the ARP message");
    free(data_with_reserve);
    
    return SYS_ERR_OK;
}

void arp_register(
    ARP* arp, ip_addr_t ip, mac_addr mac 
) {
    assert(arp);
    khint_t key;
    
    // Try to find if it already exists
    if (kh_get(ip_mac, arp->hosts, ip) != kh_end(arp->hosts)) { // Already exists
        ARP_WARN("The IP-MAC pair already exists!");
        print_ip_address(ip);
        print_mac_address(&mac);
    }
    // ALARM: Should I lock it before kh_get, if thread A is changing the hash table while thread
    // B is reading it, will it cause problem ? 
    pthread_mutex_lock(&arp->mutex);

    int ret;
    key = kh_put(ip_mac, arp->hosts, ip, &ret); 
    if (!ret) { // Can't put key into hash table
        kh_del(ip_mac, arp->hosts, key);
        ARP_ERR("Can't add a new key to IP-MAC hash table");
    }
    // Set the value of key
    kh_value(arp->hosts, key) = mac;

    pthread_mutex_unlock(&arp->mutex);
}

errval_t arp_lookup_mac(
    ARP* arp, ip_addr_t ip, mac_addr* ret_mac
) {
    assert(arp && ret_mac);

    khint_t key = kh_get(ip_mac, arp->hosts, ip);
    if (key == kh_end(arp->hosts)) { //Doesn't exist
        ARP_NOTE("The MAC address of wanted IP doesn't exist!");
        print_ip_address(ip);
        return NET_ERR_IPv4_NO_MAC_ADDRESS;
    }
    *ret_mac = kh_value(arp->hosts, key);
    return SYS_ERR_OK;
}

errval_t arp_unmarshal(
    ARP* arp, uint8_t* data, size_t size
) {
    errval_t err;
    ARP_VERBOSE("Enter");
    assert(size == sizeof(struct arp_hdr));
    struct arp_hdr* packet = (struct arp_hdr*) data;

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
        ARP_INFO("This ARP request isn't for us");
        return NET_ERR_ARP_WRONG_IP_ADDRESS;
    }

    // 2. Register the IP-MAC pair
    mac_addr  src_mac = ntoh6(packet->eth_src);
    ip_addr_t src_ip  = ntohl(packet->ip_src);
    // mac_addr  dst_mac = ntoh6(packet->eth_dst);
    arp_register(arp, src_ip, src_mac);

    uint16_t type = ntohs(packet->opcode);
    switch (type) {
    case ARP_TYPE_REPLY:
        ARP_VERBOSE("received a ARP reply packet");
        // arp_register(arp, src_ip, src_mac);
        break;
    case ARP_TYPE_REQUEST:
        ARP_VERBOSE("received a ARP request packet");
        // assert(maccmp(dst_mac, MAC_NULL) || maccmp(dst_mac, MAC_BROADCAST));
        err = arp_send(arp, ARP_OP_REP, src_ip, src_mac);
        RETURN_ERR_PRINT(err, "Can't send ARP reply");
        break;
    default:
        LOG_ERR("Unknown packet type for ARP: 0x%x", type);
        return NET_ERR_ETHER_UNKNOWN_TYPE;
    }

    ARP_VERBOSE("Exit");
    return SYS_ERR_OK;
}