#ifndef _ETHARP_H_
#define _ETHARP_H_

#include <common.h>

#define ETH_HLEN 14       /* Default size for ip header */
#define ETH_CRC_LEN 4
#define ETH_ADDR_LEN 6
#define ETH_FRAME_LEN 12

#define ETH_TYPE(hdr)  ((hdr)->type)

#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IPv4   0x0800
#define ETH_TYPE_IPv6   0x86DD

typedef struct eth_addr {
    uint8_t addr[6];
} __attribute__((__packed__)) mac_addr ;

struct eth_hdr {
    struct eth_addr dst;
    struct eth_addr src;
    uint16_t type;
} __attribute__((__packed__));

#define ARP_HW_TYPE_ETH 0x1
#define ARP_PROT_IP 0x0800
#define ARP_OP_REQ 0x1
#define ARP_OP_REP 0x2
#define ARP_HLEN_ETH 28
#define ARP_PLEN_IPV4 4

#define ARP_TYPE_REQUEST   1
#define ARP_TYPE_REPLY     2

#define MAC_BROADCAST ((mac_addr) { .addr = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } })
#define MAC_NULL      ((mac_addr) { .addr = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } })

struct arp_hdr {
    uint16_t hwtype;
    uint16_t proto;
    uint8_t hwlen;
    uint8_t protolen;
    uint16_t opcode;
    struct eth_addr eth_src;
    uint32_t ip_src;
    struct eth_addr eth_dst;
    uint32_t ip_dst;
} __attribute__((__packed__));

__BEGIN_DECLS

static inline bool maccmp(mac_addr m1, mac_addr m2) {
    for (int i = 0; i < 6; i++)
        if (m1.addr[i] != m2.addr[i]) return false;
    return true;
}

static inline mac_addr tomac(uint64_t mac) {
    uint8_t* s = (uint8_t *)&mac;
    return (mac_addr) {.addr = { s[0], s[1], s[2], s[3], s[4], s[5]}};
}

static inline uint64_t frommac(mac_addr from) {
    uint64_t to = 0;
    for (int i = 5; i >= 0; i--) {
        to <<= 8;
        to |= from.addr[i];
    }
    return to;
}

__END_DECLS

#endif  //_ETHARP_H_
