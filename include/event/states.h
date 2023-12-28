#ifndef __EVENT_STATES_H__
#define __EVENT_STATES_H__

#include <common.h>
#include <time.h>
#include <netstack/ethernet.h>
#include <device/device.h>
#include <lock_free/memorypool.h>
#include <event/threadpool.h>
#include <stdlib.h>

/* ***********************************************************
 * 
 *  State global to the whole process
 *
 ************************************************************/

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
    
    /// 
    FILE           *log_file;

} GlobalStates;

extern GlobalStates g_states;

/* ***********************************************************
 * 
 *  State local to a thread
 *
 ************************************************************/

#include <pthread.h>

// Declare the key for thread-local storage
extern pthread_key_t thread_state_key;

typedef struct local_states {
    const char  *my_name;
    pid_t        my_pid;
    FILE        *log_file;
} LocalState;

// Function prototypes
void create_thread_state_key();
void set_local_state(LocalState* new_state);
LocalState* get_local_state();

#endif // __EVENT_STATES_H__