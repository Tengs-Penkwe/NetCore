#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <stdint.h>
#include <stddef.h>
#include <lock_free/memorypool.h>

typedef struct ethernet_state Ethernet;
typedef struct memory_pool MemPool;

typedef struct {
    Ethernet *ether;

    uint8_t  *data;
    size_t    size;
    size_t    data_shift;

    MemPool  *mempool;
    bool      buf_is_from_pool;
} Frame ;

#include <netstack/arp.h>

typedef struct {
    ARP      *arp;
    ip_addr_t dst_ip;
} ARP_Request;

__BEGIN_DECLS

static inline void frame_free(Frame* frame) 
{
    assert(frame && frame->data && frame->mempool && frame->size != 0);

    void* buf = frame->data - frame->data_shift;
    if (frame->buf_is_from_pool) pool_free(frame->mempool, buf);
    else                         free(buf);
    frame->data = NULL;

    free(frame);
}

void frame_unmarshal(void* frame);
void send_arp_request(void* arp_request);

__END_DECLS

#endif // __EVENT_EVENT_H__
