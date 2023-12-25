#include <netutil/etharp.h>
#include <netutil/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>
#include <netstack/ethernet.h>
#include <netstack/ip.h>
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
    RETURN_ERR_PRINT(err, "Can't get the MAC address");
    assert(!maccmp(mac, MAC_NULL));
    ether->mac = mac;

    ETHER_INFO("My MAC address is: ");
    print_mac_address(&ether->mac);

    // 2. Set the IP address
    /// TODO: dynamic IP using DHCP
    ip_addr_t my_ip = 0x0A00020F;

    // 3. Set up the ARP: it contains the lock free hash table, which must be 128-bytes aligned
    ether->arp = aligned_alloc(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES, sizeof(ARP));
    if (ether->arp == NULL) {
        USER_PANIC("Failed to allocate the ARP");
    }
    err = arp_init(ether->arp, ether, my_ip);
    RETURN_ERR_PRINT(err, "Failed to initialize the ARP");

    // 4. Set up the IPv4
    ether->ip = calloc(1, sizeof(IP));
    if (ether->ip == NULL) {
        USER_PANIC("Failed to allocate the IP");
    }
    err = ip_init(ether->ip, ether, ether->arp, my_ip);
    RETURN_ERR_PRINT(err, "Failed to initialize the IP");

    ETHER_INFO("Ethernet Moule initialized");
    return SYS_ERR_OK;
}

errval_t ethernet_marshal(
    Ethernet* ether, mac_addr dst_mac, uint16_t type, uint8_t* data, size_t size
) {
    errval_t err;
    assert(ether && data && 
        (type == ETH_TYPE_ARP || type == ETH_TYPE_IPv4)
    ); 
    
    data -= sizeof(struct eth_hdr);
    size += sizeof(struct eth_hdr);

    struct eth_hdr* packet = (struct eth_hdr*) data;
    *packet = (struct eth_hdr){
        .src = hton6(ether->mac),
        .dst = hton6(dst_mac),
        .type = htons(type),
    };

    err = device_send(ether->device, data, size);
    RETURN_ERR_PRINT(err, "Device can't send the ethernet frame");

    return SYS_ERR_OK;
}

errval_t ethernet_unmarshal(
    Ethernet* ether, uint8_t* data, size_t size
) {
    errval_t err;
    struct eth_hdr* packet = (struct eth_hdr*) data;
    ETHER_VERBOSE("Unmarshalling %d bytes at %p", size, data);

    /// 1. Decide if the packet is for us
    mac_addr dst_mac = ntoh6(packet->dst);
    if (!(maccmp(ether->mac, dst_mac) || maccmp(dst_mac, MAC_BROADCAST))){
        ETHER_NOTE("Not a message for us, destination MAC is");
        print_mac_address(&dst_mac);
        print_mac_address(&ether->mac);
        return NET_ERR_ETHER_WRONG_MAC;
    }

    /// 2. Remove the Ethernet header and hand it to next layer
    data += sizeof(struct eth_hdr);
    size -= sizeof(struct eth_hdr);

    /// 3. Judge the packet type
    uint16_t type = ntohs(packet->type);
    switch (type) {
    case ETH_TYPE_ARP:
        ETHER_VERBOSE("Got an ARP packet");
        err = arp_unmarshal(ether->arp, data, size);
        RETURN_ERR_PRINT(err, "Error when unmarshalling ARP packet");
        break;
    case ETH_TYPE_IPv4:
        ETHER_VERBOSE("Got an IP packet");
        err = ip_unmarshal(ether->ip, data, size);
        RETURN_ERR_PRINT(err, "Error when handling IP packet");
        break;
    case ETH_TYPE_IPv6:
        ETHER_ERR("I don't support IPv6 yet");
        break;
    default:
        LOG_ERR("Unknown packet type in Enthernet Layer: %x", type);
        return NET_ERR_ETHER_UNKNOWN_TYPE;
    }

    ETHER_VERBOSE("Done Handling %d bytes at %p", size, data);
    return SYS_ERR_OK;
}