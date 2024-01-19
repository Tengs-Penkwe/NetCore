#include <netstack/netstack.h>
#include <device/device.h>

#include <event/threadpool.h>
#include <event/event.h>
#include <event/timer.h>
#include <event/memorypool.h>
#include <event/states.h>
#include <event/signal.h>

#include <sys/syscall.h>   //syscall
#include <errno.h>         //strerror

#include "driver.h"        //localserver 
                           
static void driver_exit(int signum) __attribute__((noreturn));

static errval_t signal_set_handler(void) {

    // Setup SIGINT handler
    if (signal(SIGINT, driver_exit) == SIG_ERR) {
        const char *error_msg = strerror(errno);
        LOG_FATAL("Unable to set signal handler for SIGINT: %s", error_msg);
        return EVENT_ERR_SIGNAL_INIT;
    }
    
    // Setup SIGTERM handler
    if (signal(SIGTERM, driver_exit) == SIG_ERR) {
        const char *error_msg = strerror(errno);
        LOG_FATAL("Unable to set signal handler for SIGTERM: %s", error_msg);
        return EVENT_ERR_SIGNAL_INIT;
    }

    EVENT_NOTE("The handler for SIGINT and SIGTERM is set to driver_exit");
    return SYS_ERR_OK;
}

#include "ketopt.h"

static ko_longopt_t longopts[] = {
    { "help",      ko_no_argument,       'h' },
    { "version",   ko_no_argument,       'v' },
    { "tap-path",  ko_optional_argument,  0  },
    { "tap-name",  ko_optional_argument,  0  },
    { "workers",   ko_optional_argument,  0  },
    { "log-level", ko_optional_argument,  0  },
    { "log-file",  ko_optional_argument,  0  },
    { "log-ansi",  ko_optional_argument,  0  },
    { "queue-size",ko_optional_argument,  0  },
    { NULL,        0,                     0  }
};

int main(int argc, char *argv[]) {
    errval_t err;

    ketopt_t opt = KETOPT_INIT;
    int c;
    char *tap_path = "/dev/net/tun", *tap_name = "tap0";
    int workers = 8; // default number of workers
    char *log_file_name = "/var/log/NetCore/output.json";
    int log_level = LOG_LEVEL_VERBOSE; // default log level
    bool ansi_log = false;
    rlim_t queue_size = 256273;     // The default by ulimit on my machine is 256273, thus we don't need root to run this program 

    while ((c = ketopt(&opt, argc, argv, 1, "ho:v", longopts)) >= 0) {
        switch (c) {
        case 'h': // Help
            printf("Usage: %s [options]\n", argv[0]);
            return 0;
        case 'v': // Version
            printf("Version 0.0.1\n");
            return 0;
        case 0: // Long options without a short equivalent
            if (opt.longidx == 2) { // tap-path
                tap_path = opt.arg;
            } else if (opt.longidx == 3) { // tap-name
                tap_name = opt.arg;
            } else if (opt.longidx == 4) { // workers
                workers = atoi(opt.arg);
            } else if (opt.longidx == 5) { // log-level
                log_level = atoi(opt.arg);
            } else if (opt.longidx == 6) { // log-file
                log_file_name = opt.arg;
            } else if (opt.longidx == 7) { // log-ansi
                ansi_log = true;
            } else if (opt.longidx == 8) { // size of signal queue
                queue_size = atoi(opt.arg);
            }
            break;
        case '?': // Unknown option
            printf("What do you want ?");
            return 1;
        default:
            break;
        }
    }
    
    // 1. Initialize the log system
    FILE *log_file = NULL;

    char err_msg[128];
    err = log_init(log_file_name, (enum log_level)log_level, ansi_log, &log_file);
    if (err_no(err) == EVENT_LOGFILE_CREATE)
    {
        sprintf(err_msg, "\x1B[1;91mCan't Initialize open the log file: %s, use the standard output instead\x1B[0m\n", log_file_name);
        log_file = stdout;
        fwrite(err_msg, sizeof(char), strlen(err_msg), log_file);
    }
    else if (err_is_fail(err))
    {
        sprintf(err_msg, "\x1B[1;91mCan't Initialize the log system: %s\x1B[0m\n", log_file_name);
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return -1;
    }
    assert(log_file);
    g_states.log_file = log_file;
    // After this point, we can use log

    // 2. Initialize the thread state key (LocalState)
    create_thread_state_key();

    LocalState *master = calloc(1, sizeof(LocalState));
    *master = (LocalState) {
        .my_name   = "Master",
        .my_pid    = syscall(SYS_gettid),
        .log_file  = log_file,
        .my_state  = NULL,
    };
    set_local_state(master);

    // 3. Initialize the signal set (the timer thread use the signal to wake up)
    err = signal_init(queue_size);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the signals");
        return -1;
    }

    err = signal_set_handler();
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Set the signal handler");
        return -1;
    }

    // 4. Initialize the network device
    NetDevice* device = calloc(1, sizeof(NetDevice));
    assert(device);
    err = device_init(device, tap_path, tap_name);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the Network Device");
        return -1;
    }
    g_states.device = device;

    // pass configuration to the network module through global states
    g_states.max_workers_for_single_tcp_server = workers;
    g_states.max_workers_for_single_udp_server = workers;

    // 5. Initialize the netstack module
    NetStack* net = calloc(1, sizeof(NetStack));
    assert(net);
    err = netstack_init(net, device);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize Network Module");
        return -1;
    }
    g_states.netstack = net;

    // 6. Initialize the memory pool
    MemPool* mempool = aligned_alloc(ATOMIC_ISOLATION, sizeof(MemPool));
    memset(mempool, 0x00, sizeof(MemPool));
    err = mempool_init(mempool, MEMPOOL_BYTES, MEMPOOL_AMOUNT);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the memory mempool");
        return -1;
    }
    g_states.mempool = mempool;

    // 7. Initialize the thread pool
    err = thread_pool_init(workers);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the thread mempool");
        return -1;
    }
    g_states.threadpool = &g_threadpool;

    // 8. Initialize the timer thread (timed event)
    err = timer_thread_init(g_states.timer);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the Timer");
        return -1;
    }
    g_states.timer_count = TIMER_NUM;

    // 9. Initialize the IPC thread to handle the RPC
    IPC* ipc = calloc(1, sizeof(IPC));
    err = ipc_thread_init(ipc);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the IPC");
        return -1;
    }
    g_states.ipc = ipc;

    // 10. Enter the main loop to receive the packets
    err = device_loop(device, net, mempool);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Bad thing happened in the device, loop going to shutdown!");
        return -1;
    }
    
    driver_exit(0);
}

void driver_exit(int signum) {
    (void) signum;

    ipc_thread_destroy(g_states.ipc);

    netstack_destroy(g_states.netstack);

    device_close(g_states.device);

    mempool_destroy(g_states.mempool);

    timer_thread_destroy(g_states.timer);
    // Prior to the thread pool destrcution, because there may be some timed event

    thread_pool_destroy();

    LOG_NOTE("Bye Bye !");
    
    log_close(g_states.log_file);

    exit(EXIT_SUCCESS);
}