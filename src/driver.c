#include <driver.h>
#include <netstack/ethernet.h>
#include <device/device.h>

#include <event/threadpool.h>
#include <event/event.h>
#include <event/timer.h>

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    errval_t err;

    NetDevice* device = calloc(1, sizeof(NetDevice));
    assert(device);
    //TODO: Parse it from command line
    err = device_init(device, "/dev/net/tun", "tap0");
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the Network Device");
    }

    Ethernet* ether = calloc(1, sizeof(Ethernet));
    assert(ether);
    err = ethernet_init(device, ether);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize Network Module");
    }

    err = thread_pool_init();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the thread pool");
    }

    err = timer_thread_init();
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the Timer");
    }

    device_loop(device, ether);

    LOG_ERR("Ending !");

    //TODO: join all the worker in thread pool
    //TODO: Ethernet Destroy
    thread_pool_destroy();
    close(device->tap_fd);
    return 0;
}