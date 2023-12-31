#include <device/device.h>
#include <driver.h>
#include <netstack/network.h>
#include <netutil/dump.h>
#include <netutil/htons.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <poll.h>

#include <unistd.h>
#include <errno.h>      //strerror
#include <stdio.h>      //perror
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
        const char *error_msg = strerror(errno);
        DEVICE_FATAL("Failed to open %s, because %s", tap_path, error_msg);
        return NET_ERR_DEVICE_INIT;
    }

    // Flags: IFF_TUN   - TUN device (no Ethernet headers) 
    //        IFF_TAP   - TAP device  
    //        IFF_NO_PI - Do not provide packet information  
    struct ifreq ifr = { 0 };
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI; 
    strncpy(ifr.ifr_name, tap_name, IFNAMSIZ);

    if (ioctl(tap_fd, TUNSETIFF, (void *) &ifr) < 0) {
        const char *error_msg = strerror(errno);
        DEVICE_FATAL("ioctl(TUNSETIFF): %s", error_msg);
        close(tap_fd);
        return NET_ERR_DEVICE_INIT;
    }

    DEVICE_NOTE("TAP device %s opened, tapfd = %d", ifr.ifr_name, tap_fd);

    *device = (NetDevice) {
        .tap_fd       = tap_fd,
        .ifr          = ifr,
        .recvd        = 0,
        .fail_process = 0,
        .sent         = 0,
        .fail_sent    = 0,
        .start_time   = { 0 },
    };
    clock_gettime(CLOCK_REALTIME, &device->start_time);
    
    char start_time_str[64];
    
    clock_gettime(CLOCK_REALTIME, &device->start_time);
    struct tm *tm_info = localtime(&device->start_time.tv_sec);
    strftime(start_time_str, 64, "%Y-%m-%d %H:%M:%S", tm_info);

    DEVICE_NOTE("Device initialization started at %s", start_time_str);

    return SYS_ERR_OK;
}

void device_close(NetDevice* device) {
    assert(device);
    
    // Close the TAP device
    assert(device->tap_fd >= 0);
    close(device->tap_fd);
    DEVICE_NOTE("Closed TAP device %s (fd: %d)", device->ifr.ifr_name, device->tap_fd);
    
    // Record the end time
    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);

    // Calculate elapsed time
    double elapsed_time = (end_time.tv_sec - device->start_time.tv_sec) +
                          (end_time.tv_nsec - device->start_time.tv_nsec) / 1E9;

    // Convert start and end times to human-readable strings
    char start_time_str[64], end_time_str[64];
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", localtime(&device->start_time.tv_sec));
    strftime(end_time_str, sizeof(end_time_str), "%Y-%m-%d %H:%M:%S", localtime(&end_time.tv_sec));

    DEVICE_NOTE(
        "  Device started at %s\\n"
        "  Device closed at %s\\n"
        "  Device was open for %.3f seconds.",
        start_time_str, end_time_str, elapsed_time
    );

    
    // Print device statistics
    DEVICE_NOTE(
        "Device Statistics for %s:\\n"
        "  Packets Received: %zu\\n"
        "  Packets Failed to Process: %zu\\n"
        "  Packets Sent (In-accurate): %zu\\n"
        "  Packets Failed to Send: %zu",
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

errval_t device_send(NetDevice* device, Buffer buf) {
    assert(device);

    ///TODO: if two threads write at the same time, will there be problem ?
    ssize_t written = write(device->tap_fd, buf.data, (size_t)buf.valid_size);
    if (written < 0) {
        const char *error_msg = strerror(errno);
        DEVICE_ERR("write to TAP device: %s", error_msg);
        device->fail_sent += 1;
        return NET_ERR_DEVICE_SEND;
    }
    assert((size_t)written == buf.valid_size);
    device->sent += 1;

    // printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    // printf("Written %zd bytes to TAP device\n", written);
    // dump_packet_info(buf.data);
    return SYS_ERR_OK;
}

errval_t device_get_mac(NetDevice* device, mac_addr* restrict ret_mac) {
    assert(device && ret_mac);

    // Get MAC address
    if (ioctl(device->tap_fd, SIOCGIFHWADDR, &device->ifr) < 0) {
        const char *error_msg = strerror(errno);
        DEVICE_ERR("ioctl(SIOCGIFHWADDR): %s", error_msg);
        return NET_ERR_DEVICE_GET_MAC;
    }
    *ret_mac = ntoh6(mem2mac(device->ifr.ifr_hwaddr.sa_data));

    return SYS_ERR_OK;
}

static errval_t handle_frame(NetDevice* device, NetWork* net, MemPool* mempool) {
    errval_t err;

    Ether_unmarshal* frame = malloc(sizeof(Ether_unmarshal)); assert(frame);
    *frame = (Ether_unmarshal) {
        .ether   = net->ether,
        .buf     = { 0 }, 
    };

    assert(pool_alloc(mempool, MEMPOOL_BYTES, &frame->buf) == SYS_ERR_OK);
    assert(frame->buf.valid_size == MEMPOOL_BYTES);
    assert(frame->buf.data);

    buffer_add_ptr(&frame->buf, DEVICE_HEADER_RESERVE);
    int nbytes = read(device->tap_fd, frame->buf.data, frame->buf.valid_size);
    if (nbytes <= 0) 
    {
        const char *error_msg = strerror(errno);
        DEVICE_ERR("read packet from TAP device failed: %s, but the loop continue", error_msg);
        free_ether_unmarshal(frame);
    }
    else
    {
        assert(frame->buf.valid_size == MEMPOOL_BYTES - DEVICE_HEADER_RESERVE);
        frame->buf.valid_size = nbytes;
        device->recvd += 1;

        // printf("========================================\n");
        // printf("Read %d bytes from TAP device\n", frame->buf.valid_size);
        // dump_packet_info(frame->buf.data);

        err = submit_task(MK_NORM_TASK(event_ether_unmarshal, frame));
        if (err_is_fail(err)) {

            assert(err_no(err) == EVENT_ENQUEUE_FULL);
            EVENT_WARN("The task queue is full, we need to drop this packet!");

            free_ether_unmarshal(frame);
            device->fail_process += 1;
        }
    }
    // free(buffer); Can't free it here, thread need it, must be free'd in task thread
    return SYS_ERR_OK;
}

errval_t device_loop(NetDevice* device, NetWork* net, MemPool* mempool) {
    assert(device && net);
    errval_t err;

    // Set up polling
    struct pollfd pfd[1];
    pfd[0].fd = device->tap_fd;
    pfd[0].events = POLLIN;
    
    EVENT_NOTE("Device loop starting !");

    while (true) {
        int ret = poll(pfd, 1, -1); // Wait indefinitely
        if (ret < 0) {
            const char *error_msg = strerror(errno);
            DEVICE_ERR("poll faild %s", error_msg);
            close(device->tap_fd);
            return NET_ERR_DEVICE_FAIL_POLL;
        }

        if (pfd[0].revents & POLLIN) {
            err = handle_frame(device, net, mempool);
            DEBUG_FAIL_RETURN(err, "Can't handle this frame");
        }
    }
}