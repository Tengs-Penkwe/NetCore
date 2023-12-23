#include <device/device.h>
#include <netutil/dump.h>

#include <netstack/ethernet.h>

#include <fcntl.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include <signal.h>
#include <time.h>

#include <event/threadpool.h>
#include <event/event.h>

errval_t device_init(NetDevice* device, const char* tap_path, const char* tap_name) {
    // errval_t err;

    // Open TAP device
    int tap_fd = open(tap_path, O_RDWR);
    if (tap_fd < 0) {
        LOG_ERR("opening %s", tap_path);
        perror("reason");
        return NET_ERR_DEVICE_INIT;
    }

    // Flags: IFF_TUN   - TUN device (no Ethernet headers) 
    //        IFF_TAP   - TAP device  
    //        IFF_NO_PI - Do not provide packet information  
    struct ifreq ifr = { 0 };
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI; 
    strncpy(ifr.ifr_name, tap_name, IFNAMSIZ);

    if (ioctl(tap_fd, TUNSETIFF, (void *) &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(tap_fd);
        return NET_ERR_DEVICE_INIT;
    }

    DEVICE_INFO("TAP device %s opened, tapfd = %d", ifr.ifr_name, tap_fd);

    *device = (NetDevice) {
        .tap_fd = tap_fd,
        .ifr    = ifr,
    };

    return SYS_ERR_OK;
}

errval_t device_send(NetDevice* device, void* data, size_t size) {
    assert(device && data && size);
    ssize_t written = write(device->tap_fd, data, size);
    if (written < 0) {
        perror("write to TAP device");
        return NET_ERR_DEVICE_SEND;
    }
    assert((size_t)written == size);

    DEVICE_INFO("Written %zd bytes to TAP device\n", written);
    return SYS_ERR_OK;
}

errval_t device_get_mac(NetDevice* device, mac_addr* restrict ret_mac) {
    assert(device && ret_mac);

    // Get MAC address
    if (ioctl(device->tap_fd, SIOCGIFHWADDR, &device->ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)");
        return NET_ERR_DEVICE_GET_MAC;
    }
    *ret_mac = mem2mac(device->ifr.ifr_hwaddr.sa_data);

    return SYS_ERR_OK;
}

// errval_t device_get_ipv4(NetDevice* device, ip_addr_t* ret_ip) {
//     assert(device && ret_ip);
    
//     // Get IP address
//     if (ioctl(device->tap_fd, SIOCGIFADDR, device->ifr) < 0) {
//         perror("ioctl(SIOCGIFADDR)");
//         return NET_ERR_DEVICE_INIT;
//     }

//     struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
//     char ip_str[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &ipaddr->sin_addr, ip_str, INET_ADDRSTRLEN);

//     printf("IP address: %s\n", ip_str);

// }
// static void timer_handler(int sig) {
//     printf("Timer expired, sig: %d\n", sig);
// }

// static errval_t event_init(void) {
//     struct sigaction sa;
//     sa.sa_handler = &timer_handler;
//     sigaction(SIGALRM, &sa, NULL);

//     timer_t timerid;
//     struct sigevent se;
//     se.sigev_notify = SIGEV_SIGNAL;
//     se.sigev_signo = SIGALRM;

//     timer_create(CLOCK_MONOTONIC, &se, &timerid);

//     struct itimerspec its;
//     its.it_value.tv_sec = 2; // 2 seconds
//     its.it_interval.tv_sec = 20000; // Non-repeating
//     timer_settime(timerid, 0, &its, NULL);

//     return SYS_ERR_OK;
// }

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

    // err = event_init();
    // if (err_is_fail(err)) {
    //     USER_PANIC_ERR(err, "Can't Initialize Event System");
    // }

    // Set up polling
    struct pollfd pfd[1];
    pfd[0].fd = device->tap_fd;
    pfd[0].events = POLLIN;

    while (1) {
        int ret = poll(pfd, 1, -1); // Wait indefinitely
        if (ret < 0) {
            perror("poll");
            close(device->tap_fd);
            return 1;
        }

        if (pfd[0].revents & POLLIN) {
            // Data is available to read
            char* buffer = malloc(ETHER_MAX_SIZE);
            int nbytes = read(device->tap_fd, buffer, ETHER_MAX_SIZE);
            if (nbytes < 0) {
                perror("read");
            } else {
                Frame* fr = calloc(1, sizeof(Frame));
                *fr = (Frame) {
                    .ether = ether, 
                    .data  = (uint8_t*)buffer,
                    .size  = (size_t)nbytes,
                };
                submit_task(frame_receive, fr);
                // err = ethernet_unmarshal(ether, (uint8_t*)buffer, nbytes);
                // if (err_is_fail(err)) {
                //     DEBUG_ERR(err, "We meet an error when processing this frame, but the process continue");
                // }
                // ... process the data ...
            }
            // free(buffer); Can't free it here, thread need it, must be free'd in task thread
        }
    }

    //TODO: Ethernet Destroy
    thread_pool_destroy();
    close(device->tap_fd);
    return 0;
}