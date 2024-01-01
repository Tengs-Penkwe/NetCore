#ifndef __ETHARP_H__
#define __ETHARP_H__

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
} mac_addr ;
static_assert(sizeof(mac_addr) == ETH_ADDR_LEN, "mac_addr must be 6 bytes long");  

typedef enum mac_type {
    MAC_TYPE_UNICAST,
    MAC_TYPE_MULTICAST,
    MAC_TYPE_BROADCAST,
    MAC_TYPE_NULL
} mac_type_t;

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
    return memcmp(m1.addr, m2.addr, sizeof(mac_addr)) == 0;
}

static inline mac_addr tomac(uint64_t mac) {
    return (mac_addr) {
        .addr = {
            (uint8_t)(mac & 0xFF),
            (uint8_t)((mac >> 8) & 0xFF),
            (uint8_t)((mac >> 16) & 0xFF),
            (uint8_t)((mac >> 24) & 0xFF),
            (uint8_t)((mac >> 32) & 0xFF),
            (uint8_t)((mac >> 40) & 0xFF)
        }
    };
}

static inline uint64_t frommac(mac_addr from) {
    uint64_t to;
    memcpy(&to, &from, sizeof(mac_addr));
    return (to & 0x0000FFFFFFFFFFFF);    // Mask to ensure upper bytes are zero
}

static inline mac_addr mem2mac(void* mac) {
    mac_addr result;
    memcpy(result.addr, mac, 6); // Assuming mac_addr.addr is an array of 6 uint8_t
    return result;
}

static inline mac_addr voidptr2mac(void* mac_as_ptr) {
    uint64_t s = (uint64_t)mac_as_ptr;
    return tomac(s);
}

static inline bool mac_is_ndp(mac_addr mac) {
    return mac.addr[0] == 0x33 && mac.addr[1] == 0x33;
}

static inline enum mac_type get_mac_type(const mac_addr addr) {
    if (addr.addr[0] & 0x01) {
        return MAC_TYPE_MULTICAST;
    }
    if (memcmp(addr.addr, "\xFF\xFF\xFF\xFF\xFF\xFF", ETH_ADDR_LEN) == 0) {
        return MAC_TYPE_BROADCAST;
    }
    if (memcmp(addr.addr, "\x00\x00\x00\x00\x00\x00", ETH_ADDR_LEN) == 0) {
        return MAC_TYPE_NULL;
    }
    return MAC_TYPE_UNICAST;
}

__END_DECLS

#endif  //_ETHARP_H_
