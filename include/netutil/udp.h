#ifndef _UDP_H_
#define _UDP_H_

#include <netutil/ip.h>

typedef uint16_t udp_port_t;

#define UDP_LEN_MIN    8
#define UDP_LEN_MAX   (0xFFFF - 14 - 20 - 8)

/** UDP header **/
#define UDP_HLEN 8
struct udp_hdr {
    uint16_t src;
    uint16_t dest;  /* src/dest UDP ports */
    uint16_t len;
    uint16_t chksum;
} __attribute__((__packed__));

#endif //_UDP_H_
