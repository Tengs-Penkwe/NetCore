#include <driver.h>
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
#include <lock_free/memorypool.h>

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
        .recvd  = 0,
        .fail_process = 0,
        .sent   = 0,
        .fail_sent = 0,
    };

    return SYS_ERR_OK;
}

void device_close(NetDevice* device) {
    assert(device);
    
    // Close the TAP device
    assert(device->tap_fd >= 0);
    close(device->tap_fd);
    DEVICE_ERR("Closed TAP device %s (fd: %d)\n", device->ifr.ifr_name, device->tap_fd);

    // Print device statistics
    DEVICE_ERR(
        "Device Statistics for %s:\n"
        "  Packets Received: %zu\n"
        "  Packets Failed to Process: %zu\n"
        "  Packets Sent: %zu\n"
        "  Packets Failed to Send: %zu\n",
        device->ifr.ifr_name,
        device->recvd,
        device->fail_process,
        device->sent,
        device->fail_sent
    );

    memset(device, 0, sizeof(NetDevice));
    free(device);
    device = NULL;
}

errval_t device_send(NetDevice* device, void* data, size_t size) {
    assert(device && data && size);

    ///TODO: if two threads write at the same time, will there be problem ?
    ssize_t written = write(device->tap_fd, data, size);
    if (written < 0) {
        perror("write to TAP device");
        device->fail_sent += 1;
        return NET_ERR_DEVICE_SEND;
    }
    assert((size_t)written == size);
    device->sent += 1;

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

static errval_t handle_frame(NetDevice* device, Ethernet* ether, MemPool* mempool) {
    errval_t err;

    Frame* frame = calloc(1, sizeof(Frame));
    assert(frame);
    *frame = (Frame) {
        .ether   = ether,
        .data    = NULL,
        .data_shift = 0,
        .size    = 0,
        .mempool = mempool,
        .buf_is_from_pool = true,
    };

    err = pool_alloc(mempool, (void**)&frame->data);
    if (err_no(err) == EVENT_MEMPOOL_EMPTY) {
        EVENT_ERR("We don't have more memory in the mempool !");

        assert(frame->data == NULL);
        frame->data = malloc(MEMPOOL_BYTES);
        frame->buf_is_from_pool = false;

    } else if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Shouldn't happen");
    }

    int nbytes = read(device->tap_fd, frame->data + DEVICE_HEADER_RESERVE, ETHER_MAX_SIZE);
    if (nbytes <= 0) {
        perror("read packet from TAP device failed, but the loop continue");
        frame_free(frame);
    } else {
        frame->data_shift = DEVICE_HEADER_RESERVE;
        frame->data  += frame->data_shift;
        frame->size  = (size_t)nbytes,

        device->recvd += 1;

        // printf("========================================\n");
        // printf("Read %d bytes from TAP device\n", frame->size);
        // dump_packet_info(frame->data);

        err = submit_task(MK_TASK(frame_unmarshal, frame));
        if (err_is_fail(err)) {

            assert(err_no(err) == EVENT_ENQUEUE_FULL);
            EVENT_WARN("The task queue is full, we need to drop this packet!");

            frame_free(frame);
            device->fail_process += 1;
        }
    }
    // free(buffer); Can't free it here, thread need it, must be free'd in task thread
    return SYS_ERR_OK;
}

errval_t device_loop(NetDevice* device, Ethernet* ether, MemPool* mempool) {
    assert(device && ether);
    errval_t err;

    // Set up polling
    struct pollfd pfd[1];
    pfd[0].fd = device->tap_fd;
    pfd[0].events = POLLIN;

    while (true) {
        int ret = poll(pfd, 1, -1); // Wait indefinitely
        if (ret < 0) {
            perror("poll");
            close(device->tap_fd);
            return NET_ERR_DEVICE_FAIL_POLL;
        }

        if (pfd[0].revents & POLLIN) {
            err = handle_frame(device, ether, mempool);
            RETURN_ERR_PRINT(err, "Can't handle this frame");
        }
    }
}