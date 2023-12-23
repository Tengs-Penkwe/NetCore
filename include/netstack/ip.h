#ifndef __VNET_IP_H__
#define __VNET_IP_H__

#include <netutil/ip.h>
#include "ethernet.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

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

// Default bucket size, can have more
#define IP_DEFAULT_MSG       1024
#define IP_DEFAULT_BND       256

// Use source IP + Sequence Number as hash table key
#define MSG_KEY(src_ip, seqno) (uint64_t)((uint64_t)src_ip | ((uint64_t)seqno << 32))

/// @brief Presentation of an IP Message
typedef struct ip_message {
    struct ip_state* ip;      ///< Global IP state

    uint8_t proto;  ///< Protocal over IP
    uint16_t id;    ///< Message ID

    // struct deferred_event defer;    ///< Use ip->ws, controls the dropping

    uint32_t whole_size;  ///< Size of the whole message
    uint32_t alloc_size;  // Record how much space does the data pointer holds
    void    *data;        ///< Copy of the message
    union {
        struct {
            uint32_t  size;  //TODO: This field is based on an assumption: we won't receive a same packet twice
            int       times_to_live;
            ip_addr_t src_ip;
        } recvd ;
        struct {        
            uint32_t  size;  
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

    uint16_t seg_count;  ///< Ensure the sent message have unique ID

    // struct waitset* ws;            ///< Controls the timing

    ip_addr_t ip;
    // collections_hash_table* recv_messages;  ///< For an IP address
    // collections_hash_table* send_messages;  ///< For an IP address
} IP;

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ip
);

errval_t ip_marshal(    
    IP* ip, ip_addr_t dst_ip, uint8_t proto, const void* data, const size_t size
);

errval_t ip_unmarshal(
    IP* ip, void* addr, size_t size
);

#endif  //__VNET_IP_H__
