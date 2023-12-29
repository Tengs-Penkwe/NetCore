#ifndef  __IP_GATHER_H__
#define  __IP_GATHER_H__

#include <stdatomic.h> 
#include <netstack/ip.h>
#include "kavl-lite.h"  // AVL tree for segmentation

/* TODO: Consider these situations
1. two workers received same segment at the same time
2. two workers handle the duplicate last segments at the same time
3. A modifies the hash table whill B is reading it
4. A reads when the hash table is resizing
*/

/***************************************************
* Structure to deal with IP segmentation
* Shoud be private to user
**************************************************/
#define SIZE_DONT_KNOW  0xFFFFFFFF

typedef struct message_segment Mseg;
typedef struct message_segment {
    uint32_t  offset;
    uint8_t  *data;
    KAVLL_HEAD(Mseg) head;
} Mseg ;

#define seg_cmp(p, q) (((q)->offset < (p)->offset) - ((p)->offset < (q)->offset))

/// @brief Presentation of an IP Message
typedef struct ip_recv {
    struct ip_state *ip;     ///< Global IP state
                             
    ip_addr_t        src_ip;
    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID

    Buffer           buf;    ///< Stores the data
} IP_recv;

__BEGIN_DECLS

void drop_message(void* message);
void check_recvd_message(void* message);
errval_t ip_assemble(
    IP* ip, ip_addr_t src_ip, 
    uint8_t proto, uint16_t id, 
    Buffer buf, uint16_t offset,
    bool more_frag, bool no_frag
);
errval_t ip_handle(IP_recv* msg);

__END_DECLS

#endif  // __IP_GATHER_H__
