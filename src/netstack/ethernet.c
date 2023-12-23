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

    mac_addr mac = MAC_NULL;
    err = device_get_mac(device, &mac);
    RETURN_ERR_PRINT(err, "Can't get the MAC address");
    assert(!maccmp(mac, MAC_NULL));

    /// TODO: dynamic IP
    ip_addr_t my_ip = 0x0A00020F;

    /// Set up the ARP
    ether->arp = calloc(1, sizeof(ARP));
    if (ether->arp == NULL) {
        USER_PANIC("Failed to allocate the ARP");
    }
    err = arp_init(ether->arp, ether, my_ip);
    RETURN_ERR_PRINT(err, "Failed to initialize the ARP");

    /// Set up the IP
    // ether->ip = calloc(1, sizeof(IP));
    // if (ether->ip == NULL) {
    //     USER_PANIC("Failed to allocate the IP");
    // }
    // err = ip_init(ether->ip, ether, ether->arp, my_ip);
    // RETURN_ERR_PRINT(err, "Failed to initialize the IP");

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
    Ethernet* ether, void* data, size_t size
) {
    assert(ether && data);
    (void) size;

    return SYS_ERR_NOT_IMPLEMENTED;
}