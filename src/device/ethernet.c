#include <netutil/etharp.h>
#include <netutil/ip.h>

#include <driver/driver.h>
#include <driver/ethernet.h>

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
}

errval_t ethernet_marshal(
    Ethernet* ether, mac_addr dst_mac, uint16_t type, void* data_start
);

errval_t ethernet_send(
    Ethernet* ether, void* data, size_t size
);

errval_t ethernet_unmarshal(
    Ethernet* ether, void* data, size_t size
);