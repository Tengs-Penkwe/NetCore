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

static uint32_t part_checksum(const void *buf, uint16_t len) {
    uint32_t sum = 0;
    const uint16_t* buffer = buf;

    while (len > 1)
    {
        sum += *buffer++;
        len -= sizeof(uint16_t);
    }
    if (len == 1) {
        sum += (*(uint8_t*) buffer) << 8;
    }  
    
    return sum;
}

// return the result in 
uint16_t tcp_udp_checksum_in_net_order(const void *data_no_iph, struct pseudo_ip_header_in_net_order pheader) {

    uint16_t whole_len_no_iph = ntohs(pheader.len_no_iph);
    assert(pheader.reserved == 0);
    assert(sizeof(pheader) % 2 == 0);
    
    uint32_t sum = 0;
    sum += part_checksum(&pheader, sizeof(pheader));
    sum += part_checksum(data_no_iph, whole_len_no_iph);

    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    if (sum == 0xFFFF) sum += 1;
    
    ///TODO: why we don't need htons here ?
    return /*htons*/(uint16_t)(~sum);
}
