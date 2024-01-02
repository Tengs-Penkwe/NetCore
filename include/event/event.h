#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <stdint.h>
#include <stddef.h>
#include <event/memorypool.h>
#include <event/buffer.h>

typedef struct ethernet_state Ethernet;
typedef struct memory_pool MemPool;

typedef struct {
    Ethernet *ether;
    Buffer    buf;

} Ether_unmarshal ;

#include <netstack/arp.h>

typedef struct {
    ARP      *arp;
    uint16_t  opration;
    ip_addr_t dst_ip;
    mac_addr  dst_mac;
    Buffer    buf;
} ARP_marshal;

#include <netstack/ip.h>

/// Assumption: single thread 
// typedef struct {
//     IP_assembler *assembler;
//     IP_segment     *recv;

// } IP_assemble;

typedef struct {
    IP       *ip;
    ip_addr_t src_ip;
    uint8_t   proto;
    Buffer    buf;
} IP_handle ;

#include <netstack/icmp.h>

typedef struct {
    ICMP     *icmp;
    ip_addr_t dst_ip;
    uint8_t   type;
    uint8_t   code;
    ICMP_data field;
    Buffer    buf;

} ICMP_marshal;


__BEGIN_DECLS

static inline void free_ether_unmarshal(Ether_unmarshal* unmarshal) 
{
    assert(unmarshal);
    free_buffer(unmarshal->buf);
    free(unmarshal);
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

void event_ether_unmarshal(void* unmarshal);
void event_arp_marshal(void* marshal);
void event_icmp_marshal(void* marshal);
void event_ip_assemble(void* assemble);
void event_ipv4_handle(void* handle);

__END_DECLS

#endif // __EVENT_EVENT_H__
