#ifndef __VNET_IP_H__
#define __VNET_IP_H__
#include <stdatomic.h>      // seg_count

#include <netstack/type.h>  // Buffer
#include <netutil/ip.h>
#include "ethernet.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include <semaphore.h>  
#include "khash.h"      // Hash table for IP segmentation

// Segmentation offset should be 8 alignment
// ETHER_MTU (1500) - IP (max 60) => round down to 32
#define IP_MTU               1440

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

/***************************************************
*                 IP Message Gatherer
*  All the segmented IP messages are stored in queue
*  due to their hash value, a single thread (gatherer)
*  will handle all the messages in the queue.
****************************************************/
// The Queue for segmented IP message
#define IP_GATHERER_NUM          4
#define IP_GATHER_QUEUE_SIZE     256

// Use source IP + Sequence Number as hash table key
typedef uint64_t ip_msg_key_t ;
#define IP_MSG_KEY(src_ip, seqno) (ip_msg_key_t)((uint64_t)src_ip | ((uint64_t)seqno << 32))

__BEGIN_DECLS
// @todo: use better hash function
static inline ip_msg_key_t ip_message_hash(ip_addr_t src_ip, uint16_t seqno) {
    return (ip_msg_key_t)(((uint64_t)src_ip  + (uint64_t)seqno) % IP_GATHERER_NUM);
}
__END_DECLS

typedef struct ip_recv IP_recv;
// The hash table of IP messages
KHASH_MAP_INIT_INT64(ip_msg, IP_recv*)

typedef struct ip_gatherer {
    alignas(ATOMIC_ISOLATION)
        BdQueue       msg_queue;
    size_t            queue_size;
    sem_t             msg_come;
    pthread_t         self;
    
    struct ip_state  *ip;
    
    khash_t(ip_msg)  *recv_messages;
} IP_gatherer __attribute__((aligned(ATOMIC_ISOLATION)));

typedef struct ip_state {
    alignas(ATOMIC_ISOLATION)
        IP_gatherer        gatherers[IP_GATHERER_NUM];
    size_t                 gatherer_num;

    ip_addr_t              my_ip;
    atomic_ushort          seg_count;  ///< Ensure the sent message have unique ID

    struct ethernet_state *ether;  ///< Global Ethernet state
    struct arp_state      *arp;    ///< Global ARP state
    struct icmp_state     *icmp;
    struct udp_state      *udp;
    struct tcp_state      *tcp;

} IP __attribute__((aligned(ATOMIC_ISOLATION)));

__BEGIN_DECLS

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ip
);

void ip_destroy(
    IP* ip
);

errval_t ip_marshal(    
    IP* ip, ip_addr_t dst_ip, uint8_t proto, Buffer buf
);

errval_t ip_unmarshal(
    IP* ip, Buffer buf
);

errval_t ip_handle(
    IP* ip, uint8_t proto, ip_addr_t src_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_IP_H__
