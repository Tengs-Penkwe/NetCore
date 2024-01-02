#include <netutil/checksum.h>
#include <netutil/htons.h>

static uint32_t part_checksum_in_net_order(const void *buf, uint16_t len) {
    uint32_t sum = 0;
    const uint8_t *buffer = (const uint8_t*)buf;

    while (len > 1)
    {
        /* treat first byte as most significant, which assumes the data are in network order */
        sum += (*buffer) << 8;
        buffer++;
        /* treat second byte as least significant */
        sum += *buffer;
        buffer++;
        len -= sizeof(uint16_t);
    }
    if (len == 1) {
        sum += (*buffer) << 8;
    }  
    
    return sum;
}

uint16_t inet_checksum_in_net_order(void *dataptr, uint16_t len)
{
    uint32_t sum = 0;
    sum += part_checksum_in_net_order(dataptr, len);

    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return htons((uint16_t)(~sum));
}

uint16_t tcp_checksum_in_net_order(const void *data_no_iph, struct pseudo_ip_header_in_net_order pheader) {

    uint32_t sum = 0;
    uint16_t whole_len_no_iph;
    if (pheader.is_ipv6)
    {
        uint32_t ipv6_len_is_u32 = ntohl(pheader.ipv6.len_no_iph);
        assert(ipv6_len_is_u32 <= UINT16_MAX);
        whole_len_no_iph = (uint16_t)ipv6_len_is_u32;

        assert(memcmp(&pheader.ipv6.zero, "\x00\x00\x00", sizeof(pheader.ipv6.zero)) == 0);
        assert(sizeof(pheader.ipv6) % 2 == 0);
        sum += part_checksum_in_net_order(&pheader, sizeof(pheader.ipv6));
    }
    else
    {
        whole_len_no_iph = ntohs(pheader.ipv4.len_no_iph);
        assert(pheader.ipv4.reserved == 0);
        assert(sizeof(pheader.ipv4) % 2 == 0);
        sum += part_checksum_in_net_order(&pheader, sizeof(pheader.ipv4));
    }
    
    sum += part_checksum_in_net_order(data_no_iph, whole_len_no_iph);

    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return htons((uint16_t)(~sum));
}

uint16_t udp_checksum_in_net_order(const void *data_no_iph, struct pseudo_ip_header_in_net_order pheader) {
    uint16_t checksum = tcp_checksum_in_net_order(data_no_iph, pheader);
    return (checksum == 0x0000) ? 0xFFFF : checksum;
}
