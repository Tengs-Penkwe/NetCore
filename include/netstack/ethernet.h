#ifndef __DEV_ETHERNET_H__
#define __DEV_ETHERNET_H__

#include <stdint.h>
#include <netutil/etharp.h>
#include <event/buffer.h>

__BEGIN_DECLS

#define ETHER_MTU            1500
/// 1500 (MTU) + 14 (Header) + 4(FCS) + 4 (VLAN Tag) + 4 (QinQ Tag) => round to 256
#define ETHER_MAX_SIZE       1536

typedef struct net_device NetDevice;

typedef struct ethernet_state {
    struct net_work   *net;
    mac_addr           my_mac;
    struct net_device *device;
    struct arp_state  *arp;
    struct ip_state   *ip;
} Ethernet;

errval_t ethernet_init(
    NetDevice *device, Ethernet* ether
);

void ethernet_destroy(
    Ethernet* ether
);

errval_t ethernet_marshal(
    Ethernet* ether, mac_addr dst_mac, uint16_t type, Buffer buf
);

errval_t ethernet_unmarshal(
    Ethernet* ether, Buffer buf
);

__END_DECLS

#endif //__VNET_ETHERNET_H__