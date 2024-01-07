#ifndef __EVENT_STATES_H__
#define __EVENT_STATES_H__

#include <common.h>
#include <time.h>
#include <netstack/netstack.h>
#include <device/device.h>
#include <event/memorypool.h>
#include <event/threadpool.h>
#include <event/timer.h>
#include <stdlib.h>

/* ***********************************************************
 * 
 *  State global to the whole process
 *
 ************************************************************/

typedef struct global_states {
    NetStack       *netstack;
    NetDevice      *device;
    MemPool        *mempool;
    ThreadPool     *threadpool;

    /// @brief For TCP 
    size_t          max_workers_for_single_tcp_server;
    /// @brief For UDP
    size_t          max_workers_for_single_udp_server;

    /// @brief For Device
    size_t          recvd;   ///< How many packets have we received
    size_t          fail_process;
    size_t          sent;    // Maybe inaccurate because multi-threading
    size_t          fail_sent;
    struct timespec start_time;
    
    /// @brief For Timed Event
    Timer           timer[TIMER_NUM];
    uint8_t         timer_count;
    
    /// @brief For Log
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
    void        *my_state;  // User-defined state
} LocalState;

// Function prototypes
void create_thread_state_key();
void set_local_state(LocalState* new_state);
LocalState* get_local_state();

#endif // __EVENT_STATES_H__