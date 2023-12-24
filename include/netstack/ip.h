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

__BEGIN_DECLS

typedef struct ip_message IP_message;

/// The hash table of IP-MAC
KHASH_MAP_INIT_INT64(ip_msg, IP_message*) 

typedef uint64_t ip_msg_key_t ;
// Use source IP + Sequence Number as hash table key
#define MSG_KEY(src_ip, seqno) (ip_msg_key_t)((uint64_t)src_ip | ((uint64_t)seqno << 32))

typedef struct ip_state {
    struct ethernet_state *ether; ///< Global Ethernet state
    struct arp_state *arp;        ///< Global ARP state
    struct icmp_state *icmp;
    struct udp_state *udp;
    struct tcp_state *tcp;

    ip_addr_t ip;
    uint16_t seg_count;          ///< Ensure the sent message have unique ID

    pthread_mutex_t   recv_mutex;
    khash_t(ip_msg)  *recv_messages; 
    khash_t(ip_msg)  *send_messages;
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

__END_DECLS

#endif  //__VNET_IP_H__
