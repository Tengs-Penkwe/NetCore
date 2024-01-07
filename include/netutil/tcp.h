#ifndef _TCP_H_
#define _TCP_H_

#include <netutil/ip.h>

#define TCP_HLEN_MIN         20
#define TCP_HLEN_MAX         60

#define TCP_HLEN(hdr)        ((hdr)->data_offset * 4)

/* TCP header */
struct tcp_hdr {
    uint16_t src_port;        /* Source TCP port */
    uint16_t dest_port;       /* Destination TCP port */
    uint32_t seqno;           /* Sequence number */
    uint32_t ackno;           /* Acknowledgment number */

    // it's "little-endian", so the reserved field go first
    uint8_t  reserved    : 4; /* Reserved (0) */
    uint8_t  data_offset : 4; /* Data offset (upper 4 bits) and Reserved (lower 4 bits) */

    uint8_t  flags;           /* TCP flags */
    uint16_t window;          /* Window size */
    uint16_t chksum;          /* Checksum */
    uint16_t urgent_ptr;      /* Urgent pointer */
} __attribute__((__packed__));

/* TCP Flags */
#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20
#define TCP_ECE  0x40
#define TCP_CWR  0x80

typedef uint16_t tcp_port_t;

/// The key of a connection inside the hash table of a server
typedef struct {
    ipv6_addr_t ip;     // both for IPv4 and IPv6
    tcp_port_t  port;
} __attribute__((__packed__)) tcp_conn_key_t ;
static_assert(sizeof(tcp_conn_key_t) == 18, "The size of tcp_conn_key_t should be 18 bytes");

__BEGIN_DECLS

static inline tcp_conn_key_t ipv4_tcp_conn_key(ip_addr_t ip, tcp_port_t port) {
    return (tcp_conn_key_t) {
        .ip   = ipv4_to_ipv6(ip),
        .port = port,
    };
}

static inline tcp_conn_key_t ipv6_tcp_conn_key(ipv6_addr_t ip, tcp_port_t port) {
    return (tcp_conn_key_t) {
        .ip   = ip,
        .port = port,
    };
}

static inline tcp_conn_key_t tcp_conn_key_struct(ip_context_t ip, tcp_port_t port) {
    if (ip.is_ipv6) {
        return ipv6_tcp_conn_key(ip.ipv6, port);
    } else {
        return ipv4_tcp_conn_key(ip.ipv4, port);
    }
}

static inline uint8_t tcp_get_data_offset(const struct tcp_hdr *tcp_header) {
    return (tcp_header->data_offset >> 4) * 4;  // Data offset field is in 32-bit words
}

// Function to check if a specific TCP flag is set
static inline bool tcp_flag_is_set(uint8_t flags, uint8_t flag) {
    return (flags & flag) != 0;
}

__END_DECLS

#endif  // _TCP_H_

