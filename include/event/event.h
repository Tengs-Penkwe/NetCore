#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <stdint.h>
#include <stddef.h>

typedef struct ethernet_state Ethernet;

typedef struct {
    Ethernet *ether;
    uint8_t*  data;
    size_t    size;
} Frame ;

__BEGIN_DECLS

void frame_unmarshal(void* frame);

__END_DECLS

#endif // __EVENT_EVENT_H__
