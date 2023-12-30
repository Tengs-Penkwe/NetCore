#ifndef  __IP_GATHER_H__
#define  __IP_GATHER_H__

#include <netstack/ip.h>
#include "kavl-lite.h"  // AVL tree for segmentation
#include <lock_free/bdqueue.h>
#include <pthread.h>    // pthread_t, spinlock_t
#include <time.h>       // timer_t

/***************************************************
*         IP Message (Contains Segments)
* All the segments are stored in a AVL tree, sorted, 
* after all the segments are received, we can create 
* a single IP_recv to hold all the data and pass it
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
    IP_gatherer     *gatherer;

    ip_addr_t        src_ip;
    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID

    union {
        struct {
            uint32_t size;  ///< Size of the whole message
            uint32_t recvd;  ///< How many bytes have we received (no duplicate)
            Mseg    *seg;         ///< AVL tree of segments
        } whole;
        struct {
            uint16_t offset;  ///< Offset of the segment
            bool     no_frag;
            bool     more_frag;
            Buffer   buf;  ///< Stores the data
        } seg ;
    };

    timer_t          timer;
    int              times_to_live;
} IP_recv;

typedef struct ip_state IP;
__BEGIN_DECLS

void drop_message(void* message);
void check_recvd_message(void* message);

errval_t gather_init(
    IP_gatherer* gather, size_t queue_size, size_t id
);

void gather_destroy(
    IP_gatherer* gather
);

errval_t ip_assemble(
    IP* ip, ip_addr_t src_ip, uint8_t proto, uint16_t id, 
    Buffer buf, uint16_t offset, bool more_frag, bool no_frag
);

__END_DECLS

#endif  // __IP_GATHER_H__
