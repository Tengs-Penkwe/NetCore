#include <device/device.h>
#include <netutil/dump.h>

#include <fcntl.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

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

    DRIVER_INFO("TAP device %s opened\n", ifr.ifr_name);

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

    DRIVER_INFO("Written %zd bytes to TAP device\n", written);
    return SYS_ERR_OK;
}

errval_t device_get_mac(NetDevice* device, mac_addr* ret_mac) {
    assert(device && ret_mac);

    // Get MAC address
    if (ioctl(device->tap_fd, SIOCGIFHWADDR, device->ifr) < 0) {
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

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    errval_t err;
    NetDevice* device = calloc(1, sizeof(NetDevice));

    //TODO: Parse it from command line
    err = device_init(device, "/dev/net/tun", "tap0");
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't Initialize the network device");
    }

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
            char buffer[2048];
            int nbytes = read(device->tap_fd, buffer, sizeof(buffer));
            if (nbytes < 0) {
                perror("read");
            } else {
                // Process the data
                printf("Read %d bytes from TAP device\n", nbytes);
                dump_packet_info(buffer);
                printf("========================================\n");
                // ... process the data ...
            }
        }
    }

    close(device->tap_fd);
    return 0;
}