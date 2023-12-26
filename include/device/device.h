#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <common.h>
#include <netstack/ethernet.h>

#include <linux/if.h>   //struct ifreq

/// IPv4: Max 60, 
/// TCP : Max 60, UDP : 8, ICMP : 8
#define DEVICE_HEADER_RESERVE   128
/// Round up to 8

typedef struct net_device {
    int          tap_fd; ///< TAP device file descriptor
    struct ifreq ifr;    ///< Interface request structure used for socket ioctl's
    size_t       recvd;  ///< How many packets have we received
    size_t       fail_process;
    size_t       sent;   // Maybe inaccurate because multi-threading
    size_t       fail_sent;

} NetDevice ;

__BEGIN_DECLS

errval_t device_init(NetDevice* device, const char* tap_path, const char* tap_name);
errval_t device_send(NetDevice* device, void* data, size_t size);
errval_t device_get_mac(NetDevice* device, mac_addr* ret_mac);
errval_t device_loop(NetDevice* device, Ethernet* ether);

__END_DECLS

#endif // __DEVICE_H__
