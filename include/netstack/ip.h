#ifndef __VNET_IP_H__
#define __VNET_IP_H__
#include <stdatomic.h>      // atomic operation

#include <netstack/type.h>  // Buffer
#include <netutil/ip.h>
#include "ethernet.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"

#include <lock_free/bdqueue.h>

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

// The Queue for segmented IP message
#define IP_SEG_QUEUE_NUMBER    16
#define IP_SEG_QUEUE_SIZE      64

typedef struct ip_state {
    /// @brief The Queue of TCP message, I have spinlock here since it must be single-threaded
    alignas(ATOMIC_ISOLATION)
        BdQueue            msg_queue[IP_SEG_QUEUE_SIZE];
    atomic_flag            que_locks[IP_SEG_QUEUE_SIZE];
    size_t                 queue_num;
    size_t                 queue_size;

    struct ethernet_state *ether;  ///< Global Ethernet state
    struct arp_state      *arp;    ///< Global ARP state
    struct icmp_state     *icmp;
    struct udp_state      *udp;
    struct tcp_state      *tcp;
    ip_addr_t              my_ip;
    atomic_ushort          seg_count;  ///< Ensure the sent message have unique ID
} IP __attribute__((aligned(ATOMIC_ISOLATION)));

typedef uint64_t ip_msg_key_t;

__BEGIN_DECLS

// @todo: use better hash function
static inline ip_msg_key_t ip_message_hash(ip_addr_t src_ip, uint16_t seqno) {
    return (ip_msg_key_t)(((uint64_t)src_ip  + (uint64_t)seqno) % IP_SEG_QUEUE_SIZE);
}

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

__END_DECLS

#endif  //__VNET_IP_H__
