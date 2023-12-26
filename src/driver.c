#include <driver.h>
#include <netstack/ethernet.h>
#include <device/device.h>

#include <event/threadpool.h>
#include <event/event.h>
#include <event/timer.h>
#include <lock_free/memorypool.h>

#include <stdio.h>  //perror

// Global Structure
Driver g_driver;

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

    LOG_INFO("Signals set");
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
    { NULL,        0,                     0  }
};

int main(int argc, char *argv[]) {
    errval_t err;

    ketopt_t opt = KETOPT_INIT;
    int c;
    char *tap_path = "/dev/net/tun", *tap_name = "tap0";
    int workers = 8; // default number of workers
    char *log_file = NULL;
    int log_level = LOG_LEVEL_INFO; // default log level

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
                log_file = opt.arg;
            }
            break;
        case '?': // Unknown option
            printf("What do you want ?");
            return 1;
        default:
            break;
        }
    }

    //TODO: log initialization
    (void) log_file;
    (void) log_level;

    NetDevice* device = calloc(1, sizeof(NetDevice));
    assert(device);
    err = device_init(device, tap_path, tap_name);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the Network Device");
    }

    Ethernet* ether = calloc(1, sizeof(Ethernet));
    assert(ether);
    err = ethernet_init(device, ether);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize Network Module");
    }

    MemPool* mempool = aligned_alloc(BDQUEUE_ALIGN, sizeof(MemPool));
    err = mempool_init(mempool, MEMPOOL_BYTES, MEMPOOL_AMOUNT);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the memory mempool");
    }

    err = thread_pool_init(workers);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the thread mempool");
    }

    err = signal_init();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the signals");
    }

    err = timer_thread_init();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the Timer");
    }

    g_driver = (Driver) {
        .ether      = ether,
        .device     = device,
        .mempool    = mempool,
        .threadpool = &g_threadpool,
    };

    err = device_loop(device, ether, mempool);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Enter device loop !");
    }

    USER_PANIC("Ending TODO: free resources!");

    //TODO: join all the worker in thread pool
    //TODO: Ethernet Destroy
    thread_pool_destroy();
    close(device->tap_fd);
    return 0;
}

static void driver_exit(int signum) {
    (void) signum;
    assert(g_driver.ether);
    ethernet_destroy(g_driver.ether);

    LOG_ERR("Bye Bye !");
}