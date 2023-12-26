#include <device/device.h>
#include <netutil/dump.h>
#include <netutil/htons.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <poll.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event/event.h>
#include <event/threadpool.h>

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

    ///TODO: if two threads write at the same time, will there be problem ?
    ssize_t written = write(device->tap_fd, data, size);
    if (written < 0) {
        perror("write to TAP device");
        return NET_ERR_DEVICE_SEND;
    }
    assert((size_t)written == size);

    // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    // printf("Written %zd bytes to TAP device\n", written);
    // dump_packet_info(data);
    return SYS_ERR_OK;
}

errval_t device_get_mac(NetDevice* device, mac_addr* restrict ret_mac) {
    assert(device && ret_mac);

    // Get MAC address
    if (ioctl(device->tap_fd, SIOCGIFHWADDR, &device->ifr) < 0) {
        perror("ioctl(SIOCGIFHWADDR)");
        return NET_ERR_DEVICE_GET_MAC;
    }
    *ret_mac = ntoh6(mem2mac(device->ifr.ifr_hwaddr.sa_data));

    return SYS_ERR_OK;
}

void device_loop(NetDevice* device, Ethernet* ether) {
    assert(device && ether);

    // Set up polling
    struct pollfd pfd[1];
    pfd[0].fd = device->tap_fd;
    pfd[0].events = POLLIN;

    while (1) {
        int ret = poll(pfd, 1, -1); // Wait indefinitely
        if (ret < 0) {
            perror("poll");
            close(device->tap_fd);
            return;
        }

        if (pfd[0].revents & POLLIN) {
            // Data is available to read
            char* buffer = malloc(ETHER_MAX_SIZE + DEVICE_HEADER_RESERVE);
            int nbytes = read(device->tap_fd, buffer + DEVICE_HEADER_RESERVE, ETHER_MAX_SIZE);
            if (nbytes < 0) {
                perror("read");
            } else {
                Frame* fr = calloc(1, sizeof(Frame));
                *fr = (Frame) {
                    .ether = ether, 
                    .data  = (uint8_t*)buffer + DEVICE_HEADER_RESERVE,
                    .size  = (size_t)nbytes,
                };
                // printf("========================================\n");
                // printf("Read %d bytes from TAP device\n", fr->size);
                // dump_packet_info(fr->data);
                errval_t err = submit_task(MK_TASK(frame_unmarshal, fr));
                if (err_is_fail(err)) {
                    assert(err_no(err) == EVENT_ENQUEUE_FULL);
                    EVENT_WARN("The task queue is full, we need to drop some packet !");
                    free(fr);
                    free(buffer);
                }
            }
            // free(buffer); Can't free it here, thread need it, must be free'd in task thread
        }
    }
}