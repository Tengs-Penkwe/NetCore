#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <stdint.h>
#include <stddef.h>
#include <lock_free/memorypool.h>
#include <netstack/type.h>

typedef struct ethernet_state Ethernet;
typedef struct memory_pool MemPool;

typedef struct {
    Ethernet *ether;

    Buffer    buf;
} Frame ;

#include <netstack/arp.h>

typedef struct {
    ARP      *arp;
    uint16_t  opration;
    ip_addr_t dst_ip;
    mac_addr  dst_mac;
    Buffer    buf;
} ARP_marshal;

#include <netstack/ip.h>

typedef struct {
    IP *ip;

} IP_marshal;

#include <netstack/icmp.h>

typedef struct {
    ICMP* icmp;
    ip_addr_t dst_ip;
    uint8_t type;
    uint8_t code;
    ICMP_data field;
    Buffer buf;

} ICMP_marshal;


__BEGIN_DECLS

static inline void free_frame(Frame* frame) 
{
    assert(frame);
    free_buffer(frame->buf);
    free(frame);
}

static inline void free_icmp_marshal(ICMP_marshal* marshal)
{
    assert(marshal);
    free_buffer(marshal->buf);
    free(marshal);
}

static inline void free_arp_marshal(ARP_marshal* marshal)
{
    assert(marshal);
    free_buffer(marshal->buf);
    free(marshal);
}

void frame_unmarshal(void* frame);
void event_arp_marshal(void* marshal);
void event_icmp_marshal(void* marshal);

__END_DECLS

#endif // __EVENT_EVENT_H__
