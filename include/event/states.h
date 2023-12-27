#ifndef __EVENT_STATES_H__
#define __EVENT_STATES_H__

#include <common.h>
#include <time.h>
#include <netstack/ethernet.h>
#include <device/device.h>
#include <lock_free/memorypool.h>
#include <event/threadpool.h>

typedef struct driver {
    Ethernet    *ether;
    NetDevice   *device;  
    MemPool     *mempool;
    ThreadPool  *threadpool;

} Driver ;

typedef struct global_states {
    /// @brief For TCP 
    size_t          max_worker_for_single_tcp_server;

    Driver          driver;
    /// @brief For Device
    size_t          recvd;   ///< How many packets have we received
    size_t          fail_process;
    size_t          sent;    // Maybe inaccurate because multi-threading
    size_t          fail_sent;
    struct timespec start_time;

} GlobalStates;

extern GlobalStates g_states;

#endif // __EVENT_STATES_H__