#include <netutil/etharp.h>
#include <netutil/dump.h>
#include <netutil/htons.h>
#include <netstack/ethernet.h>
#include <netstack/arp.h>

errval_t arp_init(
    ARP* arp, Ethernet* ether, ip_addr_t ip
) {
    errval_t err;
    assert(arp && ether);
    arp->ether = ether;
    arp->ip = ip;

    err = hash_init(&arp->hosts, HS_FAIL_ON_EXIST);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of ARP");

    ARP_INFO("ARP Module initialized");
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
    assert(arp && ret_mac);
    errval_t err;
    void* macaddr_as_pointer = NULL;

    err = hash_get_by_key(&arp->hosts, ARP_HASH_KEY(ip), &macaddr_as_pointer);
    RETURN_ERR_PRINT(err, "Can't find the MAC address of given IPv4 address");

    assert(macaddr_as_pointer);
    *ret_mac = voidptr2mac(macaddr_as_pointer);

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
    arp_register(arp, src_ip, src_mac);

    uint16_t type = ntohs(packet->opcode);
    switch (type) {
    case ARP_TYPE_REPLY:
        ARP_VERBOSE("received a ARP reply packet");
        break;
    case ARP_TYPE_REQUEST:
        ARP_VERBOSE("received a ARP request packet");
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