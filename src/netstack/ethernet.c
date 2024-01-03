#include <netutil/etharp.h>
#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>
#include <netstack/ethernet.h>
#include <netstack/ip.h>
#include <netstack/ndp.h>
#include <device/device.h>

errval_t ethernet_init(
    struct net_device *device, Ethernet* ether
) {
    assert(device && ether);
    errval_t err;
    // 0. Set the device
    ether->device = device;

    // 1. Get and set the MAC address
    mac_addr mac = MAC_NULL;
    err = device_get_mac(device, &mac);
    DEBUG_FAIL_RETURN(err, "Can't get the MAC address");
    assert(!maccmp(mac, MAC_NULL));
    ether->my_mac = mac;
    
    char mac_str[18]; format_mac_address(&mac, mac_str, sizeof(mac_str));
    ETHER_NOTE("My MAC address is: %s", mac_str);

    // 2. Set the IP address
    /// TODO: dynamic IP using DHCP
    ip_addr_t   my_ipv4 = 0x0A00020F;
    ipv6_addr_t my_ipv6 = mk_ipv6(0xfe80000000000000, 0xe0dd05fffedc6aa8);
    
    char ip_str[16]; format_ipv4_addr(my_ipv4, ip_str, sizeof(ip_str));
    char ipv6_str[39]; format_ipv6_addr(my_ipv6, ipv6_str, sizeof(ipv6_str));
    ETHER_NOTE("My static IPv4 address is: %s", ip_str);
    ETHER_NOTE("My static IPv6 address is: %s", ipv6_str);

    // 3. Set up the ARP: it contains the lock free hash table, which must be 128-bytes aligned
    ether->arp = aligned_alloc(ATOMIC_ISOLATION, sizeof(ARP)); assert(ether->arp);
    memset(ether->arp, 0, sizeof(ARP));
    err = arp_init(ether->arp, ether, my_ipv4);
    DEBUG_FAIL_RETURN(err, "Failed to initialize the ARP");
    
    // 5. Set up the IP
    ether->ip = aligned_alloc(ATOMIC_ISOLATION, sizeof(IP)); assert(ether->ip);
    memset(ether->ip, 0, sizeof(IP)); 
    err = ip_init(ether->ip, ether, ether->arp, my_ipv4, my_ipv6);
    DEBUG_FAIL_RETURN(err, "Failed to initialize the IP");
    
    ETHER_NOTE("Ethernet Moule initialized");
    return SYS_ERR_OK;
}

void ethernet_destroy(
    Ethernet* ether
) {
    assert(ether);
    
    arp_destroy(ether->arp);
    ip_destroy(ether->ip);

    LOG_NOTE("Ethernet Module destroyed");
}

errval_t ethernet_marshal(
    Ethernet* ether, mac_addr dst_mac, uint16_t type, Buffer buf
) {
    errval_t err;
    assert(ether && (type == ETH_TYPE_ARP || type == ETH_TYPE_IPv4)); 
    
    buffer_sub_ptr(&buf, sizeof(struct eth_hdr));

    struct eth_hdr* packet = (struct eth_hdr*) buf.data;
    *packet = (struct eth_hdr){
        .src  = hton6(ether->my_mac),
        .dst  = hton6(dst_mac),
        .type = htons(type),
    };

    err = device_send(ether->device, buf);
    DEBUG_FAIL_RETURN(err, "Device can't send the ethernet frame");

    return SYS_ERR_OK;
}

errval_t ethernet_unmarshal(
    Ethernet* ether, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    struct eth_hdr *packet = (struct eth_hdr *)buf.data;
    ETHER_VERBOSE("Unmarshalling %d bytes", buf.valid_size);

    /// 1. Remove the Ethernet header and hand it to next layer
    buffer_add_ptr(&buf, sizeof(struct eth_hdr));

    /// 2. Decide if the packet is for us
    mac_addr dst_mac = ntoh6(packet->dst);
    switch(get_mac_type(dst_mac)) {
    case MAC_TYPE_NULL:
        ETHER_WARN("Got a NULL message");
        return NET_ERR_ETHER_NULL_MAC;
    case MAC_TYPE_MULTICAST: {
        ETHER_VERBOSE("Got a multicast message");
        if (!mac_is_ndp(dst_mac))
        {
            char mac_str[18]; format_mac_address(&dst_mac, mac_str, sizeof(mac_str));
            char my_mac_str[18]; format_mac_address(&ether->my_mac, my_mac_str, sizeof(my_mac_str));
            ETHER_NOTE("An unknown ethernet multicast for MAC %s, my MAC is %s", mac_str, my_mac_str);
            return NET_ERR_ETHER_WRONG_MAC;
        }
        else
        {
            ETHER_VERBOSE("An NDP message");
            // // err = ndp_unmarshal(ether->ndp, buf);
            // DEBUG_FAIL_RETURN(err, "Error when unmarshalling NDP packet");
        }
    }
            break;
        assert(!"unreachable");
    case MAC_TYPE_BROADCAST:
        ETHER_VERBOSE("Got a broadcast message");
        break;
    case MAC_TYPE_UNICAST: {
        if (!maccmp(ether->my_mac, dst_mac))
        {
            char mac_str[18]; format_mac_address(&dst_mac, mac_str, sizeof(mac_str));
            char my_mac_str[18]; format_mac_address(&ether->my_mac, my_mac_str, sizeof(my_mac_str));
            ETHER_NOTE("Not a message for us, destination MAC is %s, my MAC is %s", mac_str, my_mac_str);
            return NET_ERR_ETHER_WRONG_MAC;
        }
        break;
    }
    default: USER_PANIC("Unknown MAC type");
    }

    /// 3. Judge the packet type
    uint16_t type = ntohs(packet->type);
    switch (type) {
    case ETH_TYPE_ARP:
        ETHER_VERBOSE("Got an ARP packet");
        err = arp_unmarshal(ether->arp, buf);
        DEBUG_FAIL_RETURN(err, "Error when unmarshalling ARP packet");
        return err;
    case ETH_TYPE_IPv6:
        ETHER_VERBOSE("Got an IPv6 packet");
        [[fallthrough]];
    case ETH_TYPE_IPv4:
        err = ip_unmarshal(ether->ip, buf);
        DEBUG_FAIL_RETURN(err, "Error when handling IP packet");
        return err;
    default:
        LOG_ERR("Unknown packet type in Enthernet Layer: %x", type);
        return NET_ERR_ETHER_UNKNOWN_TYPE;
    }
}