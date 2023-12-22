#include <netutil/checksum.h>
#include <netutil/htons.h>

static uint16_t
lwip_standard_chksum(void *dataptr, uint16_t len)
{
    uint32_t acc;
    uint16_t src;
    uint8_t *octetptr;

    acc = 0;
    /* dataptr may be at odd or even addresses */
    octetptr = (uint8_t*)dataptr;
    while (len > 1) {
        /* declare first octet as most significant
       thus assume network order, ignoring host order */
        src = (*octetptr) << 8;
        octetptr++;
        /* declare second octet as least significant */
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0) {
        /* accumulate remaining octet */
        src = (*octetptr) << 8;
        acc += src;
    }
    /* add deferred carry bits */
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000UL) != 0) {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
    /* This maybe a little confusing: reorder sum using htons()
     instead of ntohs() since it has a little less call overhead.
     The caller must invert bits for Internet sum ! */
    return htons((uint16_t)acc);
}

/**
 * Calculate a short such that ret + dataptr[..] becomes 0
 */
uint16_t inet_checksum(void *dataptr, uint16_t len)
{
    return ~lwip_standard_chksum(dataptr, len);
}

#include <stdint.h>
#include <string.h>

static uint16_t calculate_checksum(void *buf, int len) {
    uint32_t sum = 0;
    uint16_t* buffer = buf;

    while (len > 1)
    {
        sum += *buffer++;
        len -= sizeof(uint16_t);
    }
    if (len == 1) {
        sum += *(uint8_t*) buffer;
    } 

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)(~sum);
}

// return the result in 
uint16_t tcp_udp_checksum(const void *data_no_iph, struct pseudo_ip_header pheader) {

    // Convert pseudo_header to network byte order
    struct pseudo_ip_header phdr;
    phdr.src_addr     = htonl(pheader.src_addr);
    phdr.dst_addr     = htonl(pheader.dst_addr);
    phdr.reserved     = 0;
    phdr.protocol     = pheader.protocol;
    phdr.len_no_iph   = htons(pheader.len_no_iph);

    // Calculate the sum
    static uint8_t buf[65536];
    memcpy(buf, &phdr, sizeof(phdr));
    memcpy(buf + sizeof(phdr), data_no_iph, pheader.len_no_iph );

    return calculate_checksum(buf, sizeof(phdr) + pheader.len_no_iph);
}
