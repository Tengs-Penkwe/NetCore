#ifndef __DEVICE_H__
#define __DEVICE_H__
 
#include <common.h>
#include <netstack/ethernet.h>
#include <time.h>       // For clock_gettime and struct timespec

#include <linux/if.h>   //struct ifreq
typedef struct memory_pool MemPool;
typedef struct net_stack   NetStack;

/// IPv4: Max 60, 
/// TCP : Max 60, UDP : 8, ICMP : 8
#define DEVICE_HEADER_RESERVE   128
/// Round up to 8

typedef struct net_device {
    int             tap_fd;  ///< TAP device file descriptor
    struct ifreq    ifr;     ///< Interface request structure used for socket ioctl's
    size_t          recvd;   ///< How many packets have we received
    size_t          fail_process;
    size_t          sent;    // Maybe inaccurate because multi-threading
    size_t          fail_sent;
    struct timespec start_time;
} NetDevice ;

__BEGIN_DECLS

errval_t device_init(NetDevice* device, const char* tap_path, const char* tap_name);
void     device_close(NetDevice* device);
errval_t device_send(NetDevice* device, Buffer buf);
errval_t device_get_mac(NetDevice* device, mac_addr* ret_mac);
errval_t device_loop(NetDevice* device, NetStack* net, MemPool* mempool);

__END_DECLS

#endif // __DEVICE_H__
