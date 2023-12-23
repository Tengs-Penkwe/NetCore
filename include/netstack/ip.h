#ifndef __VNET_IP_H__
#define __VNET_IP_H__

#include <netutil/ip.h>
#include "ethernet.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

#include "cc_hashtable.h"  // Hash Table
#include "cc_array.h"      // Array

// Segmentation offset should be 8 alignment
#define IP_MTU   ROUND_DOWN((ETHER_MTU - sizeof(struct ip_hdr) - 80), 8)

/// Time for Sending
// 5 Milli-Second: increases by 2
#define IP_RETRY_SEND_US     5000
/// Time for ARP : 0.5 Second
#define ARP_WAIT_US          500000
// 32 Seconds
#define IP_GIVEUP_SEND_US    32000000

/// Time for Receiving
// 0.2 Second
#define IP_RETRY_RECV_US     200000 
// 10 Seconds
#define IP_GIVEUP_RECV_US    10000000

/// @brief Presentation of an IP Message
typedef struct ip_message {
    struct ip_state *ip;     ///< Global IP state

    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID

    uint32_t         whole_size;  ///< Size of the whole message
    CC_Array        *data;   ///< All segmentation

    union {
        struct {
            int       times_to_live;
            ip_addr_t src_ip;
        } recvd ;
        struct {        
            int       retry_interval;
            ip_addr_t dst_ip;
            mac_addr  dst_mac;
        } sent;
    };
} IP_message;

typedef struct ip_state {
    struct ethernet_state* ether;  ///< Global Ethernet state
    struct arp_state* arp;         ///< Global ARP state
    struct icmp_state* icmp;
    struct udp_state* udp;
    struct tcp_state* tcp;

    uint16_t seg_count;            ///< Ensure the sent message have unique ID

    ip_addr_t ip;
    CC_HashTable  *recv_messages;
    CC_HashTable  *send_messages;
} IP;

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ip
);

errval_t ip_marshal(    
    IP* ip, ip_addr_t dst_ip, uint8_t proto, const void* data, const size_t size
);

errval_t ip_unmarshal(
    IP* ip, uint8_t* data, size_t size
);

#endif  //__VNET_IP_H__
