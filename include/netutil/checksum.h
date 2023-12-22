#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <stdint.h>

/**
 * Calculate the internet checksum according to RFC1071
 */
uint16_t inet_checksum(void *dataptr, uint16_t len);


#include <netutil/udp.h>

struct pseudo_ip_header {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t  reserved;  // This is typically 0
    uint8_t  protocol;
    uint16_t len_no_iph;
} __attribute__((packed));

/**
 * Calculate the TCP/UDP checksum according to RFC1071
 */
uint16_t tcp_udp_checksum(const void *data_no_iph, struct pseudo_ip_header pheader);

#endif

