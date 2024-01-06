#ifndef __VNET_IP_H__
#define __VNET_IP_H__
#include <stdatomic.h>      // seg_count

#include <event/buffer.h>  // Buffer
#include <netutil/ip.h>
#include "ethernet.h"
#include "arp.h"
#include "ndp.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "tcp.h"
#include <semaphore.h>  
#include "khash.h"      // Hash table for IP segmentation
#include "kavl-lite.h"  // AVL tree for segmentation
#include <pthread.h>    // pthread_t, spinlock_t
#include <time.h>       // timer_t

// Segmentation offset should be 8 alignment
// ETHER_MTU (1500) - IP (max 60) => round down to 32
#define IP_MTU               1440

/// Time for Sending
// 5 Milli-Second: increases by 2
#define IP_RETRY_SEND_US     5000
/// Time for ARP and  : 0.5 Second
#define GET_MAC_WAIT_US      500000
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
*  due to their hash value, a single thread (assembler)
*  will handle all the messages in the queue.
****************************************************/
// The Queue for segmented IP message
#define IP_ASSEMBLER_NUM            4
#define IP_ASSEMBLER_QUEUE_SIZE     256

// Use source IP + Sequence Number as hash table key
typedef uint64_t ip_msg_key_t ;
#define IP_MSG_KEY(src_ip, seqno) (ip_msg_key_t)((uint64_t)src_ip | ((uint64_t)seqno << 32))

__BEGIN_DECLS
// @todo: use better hash function
static inline ip_msg_key_t ip_message_hash(ip_addr_t src_ip, uint16_t seqno) {
    return (ip_msg_key_t)(((uint64_t)src_ip  + (uint64_t)seqno) % IP_ASSEMBLER_NUM);
}
__END_DECLS

typedef struct ip_recv IP_recv;
// The hash table of IP messages
KHASH_MAP_INIT_INT64(ip_msg, IP_recv*)

typedef struct ip_assembler {
    alignas(ATOMIC_ISOLATION)
        BdQueue       event_que;
    sem_t             event_come;
    size_t            queue_size;
    pthread_t         self;
    
    struct ip_state  *ip;
    
    khash_t(ip_msg)  *recv_messages;
} IP_assembler __attribute__((aligned(ATOMIC_ISOLATION)));

/***************************************************
*         IP Message (Contains Segments)
* All the segments are stored in a AVL tree, sorted, 
* after all the segments are received, we can create 
* a single IP_segment to hold all the data and pass it
****************************************************/
#define SIZE_DONT_KNOW  0xFFFFFFFF

typedef struct message_segment Mseg;
typedef struct message_segment {
    uint32_t         offset; ///< Offset of the segment
    Buffer           buf;    ///< Stores the data
    KAVLL_HEAD(Mseg) head;
} Mseg ;

#define seg_cmp(p, q) (((q)->offset < (p)->offset) - ((p)->offset < (q)->offset))

typedef struct ip_recv {
    IP_assembler     *assembler;
    ip_addr_t        src_ip;
    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID
                             
    uint32_t         whole_size;  ///< Size of the whole message
    uint32_t         recvd_size;  ///< How many bytes have we received (no duplicate)
    Mseg            *seg;         ///< AVL tree of segments

    timer_t          timer;
    int              times_to_live;
} IP_recv;

typedef struct ip_segment {
    IP_assembler     *assembler;
    ip_addr_t        src_ip;
    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID
                             
    uint16_t         offset;  ///< Offset of the segment
    bool             no_frag;
    bool             more_frag;
    Buffer           buf;    ///< Stores the data
} IP_segment;


typedef struct ip_state {
    alignas(ATOMIC_ISOLATION)
        IP_assembler       assemblers[IP_ASSEMBLER_NUM];
    size_t                 assembler_num;

    ip_addr_t              my_ipv4;
    atomic_ushort          seg_count;  ///< Ensure the sent message have unique ID
    ipv6_addr_t            my_ipv6;

    struct ethernet_state *ether;  ///< Global Ethernet state
    struct arp_state      *arp;    ///< Global ARP state
    struct icmp_state     *icmp;
    struct udp_state      *udp;
    struct tcp_state      *tcp;

} IP __attribute__((aligned(ATOMIC_ISOLATION)));

__BEGIN_DECLS

errval_t ip_init(
    IP* ip, Ethernet* ether, ARP* arp, ip_addr_t my_ipv4, ipv6_addr_t my_ipv6
);

void ip_destroy(
    IP* ip
);

errval_t ip_assemble(
    IP_segment* recv
);

errval_t ip_marshal(    
    IP* ip, ip_context_t dst_ip, uint8_t proto, Buffer buf
);

static inline errval_t lookup_mac(
    IP* ip, ip_context_t dst_ip, mac_addr* ret_mac
) {
    if (dst_ip.is_ipv6) {
        return ndp_lookup_mac(ip->icmp, dst_ip.ipv6, ret_mac);
    } else {
        return arp_lookup_mac(ip->arp, dst_ip.ipv4, ret_mac);
    }
}

errval_t ipv6_unmarshal(
    IP* ip, Buffer buf
);

errval_t ipv4_unmarshal(
    IP* ip, Buffer buf
);

static inline errval_t ip_unmarshal(IP* ip, Buffer buf) {
    uint8_t version =  ((struct ip_hdr*)buf.data)->version;
    switch (version) {
    case 4: return ipv4_unmarshal(ip, buf);
    case 6: return ipv6_unmarshal(ip, buf);
    default: return NET_ERR_IP_VERSION;
    }
}

errval_t ipv4_handle(
    IP* ip, uint8_t proto, ip_addr_t src_ip, Buffer buf
);

__END_DECLS

#endif  //__VNET_IP_H__
