#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <common.h>

#include <netstack/ethernet.h>
#include <device/device.h>
#include <event/threadpool.h>

// Each pieces have reserved space in the head to avoid copying
#define  MEMPOOL_BYTES       ETHER_MAX_SIZE + DEVICE_HEADER_RESERVE
// Give 2048 peices in Memory pool
#define  MEMPOOL_AMOUNT      2048

typedef struct driver {
    Ethernet    *ether;
    NetDevice   *device;  
    MemPool     *mempool;
    ThreadPool  *threadpool;

} Driver ;

extern Driver g_driver;

__BEGIN_DECLS

int main(int argc, char* argv[]);

__END_DECLS

#endif // __DRIVER_H__