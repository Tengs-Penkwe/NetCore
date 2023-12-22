#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <common.h>
#include <netutil/etharp.h>

#include <linux/if.h>   //struct ifreq

typedef struct net_device {
    int tap_fd; ///< TAP device file descriptor
    struct ifreq ifr;

} NetDevice ;

__BEGIN_DECLS

errval_t device_init(NetDevice* device, const char* tap_path, const char* tap_name);
errval_t device_send(NetDevice* device, void* data, size_t size);
errval_t device_get_mac(NetDevice* device, mac_addr* ret_mac);
// errval_t device_get_ipv4(NetDevice* device, ip_addr_t* ret_ip);
int main(int argc, char* argv[]);

__END_DECLS
#endif // __DRIVER_H__
