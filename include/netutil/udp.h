#ifndef _UDP_H_
#define _UDP_H_

#include <netutil/ip.h>

typedef uint16_t udp_port_t;

/** UDP header **/
#define UDP_HLEN 8
struct udp_hdr {
    uint16_t src;
    uint16_t dest;  /* src/dest UDP ports */
    uint16_t len;
    uint16_t chksum;
} __attribute__((__packed__));

/// The key of a connection inside the hash table of a server
typedef struct udp_conn_key {
    ipv6_addr_t ip;     // both for IPv4 and IPv6
    udp_port_t  port;
} __attribute__((__packed__)) udp_conn_key_t ;
static_assert(sizeof(udp_conn_key_t) == 18, "The size of udp_conn_key_t should be 18 bytes");

#endif //_UDP_H_
