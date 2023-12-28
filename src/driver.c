#include <driver.h>
#include <netstack/network.h>
#include <device/device.h>

#include <event/threadpool.h>
#include <event/event.h>
#include <event/timer.h>
#include <lock_free/memorypool.h>
#include <event/states.h>

#include <stdio.h>         //perror
#include <sys/syscall.h>   //syscall
                           
static void driver_exit(int signum);

static errval_t signal_init(void) {
    // Initialize the signal set
    sigset_t set;
    sigemptyset(&set);
    // sigaddset(&set, SIG_TELL_TIMER);
    sigaddset(&set, SIG_TIGGER_SUBMIT);

    // Block The signal for timer
    // Note: we are in main thread !
    if (sigprocmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("sigprocmask");
        return SYS_ERR_FAIL;
    }

    // Setup SIGINT handler
    if (signal(SIGINT, driver_exit) == SIG_ERR) {
        perror("Unable to set signal handler for SIGINT");
        return SYS_ERR_FAIL;
    }
    
    // Setup SIGTERM handler
    if (signal(SIGTERM, driver_exit) == SIG_ERR) {
        perror("Unable to set signal handler for SIGTERM");
        return SYS_ERR_FAIL;
    }

    EVENT_NOTE("Signals set");
    return SYS_ERR_OK;
}

#include "ketopt.h"

ko_longopt_t longopts[] = {
    { "help",      ko_no_argument,       'h' },
    { "version",   ko_no_argument,       'v' },
    { "tap-path",  ko_optional_argument,  0  },
    { "tap-name",  ko_optional_argument,  0  },
    { "workers",   ko_optional_argument,  0  },
    { "log-level", ko_optional_argument,  0  },
    { "log-file",  ko_optional_argument,  0  },
    { "log-ansi",  ko_optional_argument,  0  },
    { NULL,        0,                     0  }
};

int main(int argc, char *argv[]) {
    errval_t err;

    ketopt_t opt = KETOPT_INIT;
    int c;
    char *tap_path = "/dev/net/tun", *tap_name = "tap0";
    int workers = 8; // default number of workers
    char *log_file_name = "/var/log/TCP-IP/output.json";
    int log_level = COMMON_LOG_LEVEL; // default log level
    bool ansi_log = false;

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
            }
            break;
        case '?': // Unknown option
            printf("What do you want ?");
            return 1;
        default:
            break;
        }
    }
    
    FILE *log_file = NULL;

    err = log_init(log_file_name, (enum log_level)log_level, ansi_log, &log_file);
    if (err_no(err) == EVENT_LOGFILE_CREATE)
    {
        printf("\x1B[1;91mCan't Initialize open the log file: %s, use the standard output instead\x1B[0m\n", log_file);
        log_file = stdout;
    }
    else if (err_is_fail(err))
    {
        printf("\x1B[1;91mCan't Initialize the log system: %s\x1B[0m\n", log_file);
        return -1;
    }
    assert(log_file);
    g_states.log_file = log_file;
    // After this point, we can use log

    //TODO: free it ?
    create_thread_state_key();

    LocalState *master = calloc(1, sizeof(LocalState));
    *master = (LocalState) {
        .my_name   = "Master",
        .my_pid    = syscall(SYS_gettid),
        .log_file  = log_file,
    };
    set_local_state(master);

    NetDevice* device = calloc(1, sizeof(NetDevice));
    assert(device);
    err = device_init(device, tap_path, tap_name);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the Network Device");
        return -1;
    }
    g_states.device = device;

    g_states.max_workers_for_single_tcp_server = workers;
    g_states.max_workers_for_single_udp_server = workers;

    NetWork* net = calloc(1, sizeof(NetWork));
    assert(net);
    err = network_init(net, device);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize Network Module");
        return -1;
    }
    g_states.network = net;

    MemPool* mempool = aligned_alloc(ATOMIC_ISOLATION, sizeof(MemPool));
    memset(mempool, 0x00, sizeof(MemPool));
    err = mempool_init(mempool, MEMPOOL_BYTES, MEMPOOL_AMOUNT);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the memory mempool");
        return -1;
    }
    g_states.mempool = mempool;

    err = thread_pool_init(workers);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the thread mempool");
        return -1;
    }
    g_states.threadpool = &g_threadpool;

    err = signal_init();
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the signals");
        return -1;
    }

    err = timer_thread_init();
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't Initialize the Timer");
        return -1;
    }

    err = device_loop(device, net, mempool);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Bad thing happened in the device, loop going to shutdown!");
        return -1;
    }
    
    driver_exit(0);
    return -1;
}

static void driver_exit(int signum) {
    (void) signum;
    LOG_ERR("Ending TODO: free resources!");


    network_destroy(g_states.network);

    device_close(g_states.device);

    mempool_destroy(g_states.mempool);

    thread_pool_destroy();

    timer_thread_destroy();

    LOG_NOTE("Bye Bye !");
    
    log_close(g_states.log_file);

    exit(EXIT_SUCCESS);
}