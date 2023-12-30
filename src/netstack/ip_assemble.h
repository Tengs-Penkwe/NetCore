#ifndef  __IP_GATHER_H__
#define  __IP_GATHER_H__

#include <netstack/ip.h>
#include <lock_free/bdqueue.h>
                        
__BEGIN_DECLS

void drop_recvd_message(void* message);
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
