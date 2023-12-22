#ifndef _TCP_H_
#define _TCP_H_

#include <netutil/ip.h>

#define TCP_HLEN(packet)     (((packet->data_offset & 0xF0) >> 4) * 4)
#define TCP_RSVR(packet)     ((packet->data_offset) & 0x0F)
#define TCP_HLEN_MIN         20
#define TCP_HLEN_MAX         60
#define TCPH_SET_LEN(offset) (uint8_t)(((offset / 4) << 4) & 0xF0)

/**
 * TCP header
 */
struct tcp_hdr {
    uint16_t src_port;   /* Source TCP port */
    uint16_t dest_port;  /* Destination TCP port */
    uint32_t seqno;      /* Sequence number */
    uint32_t ackno;      /* Acknowledgment number */
    uint8_t data_offset; /* Data offset (upper 4 bits) and Reserved (lower 4 bits) */
    uint8_t flags;       /* TCP flags */
    uint16_t window;     /* Window size */
    uint16_t chksum;     /* Checksum */
    uint16_t urgent_ptr; /* Urgent pointer */
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

typedef uint16_t trans_port_t;

__BEGIN_DECLS

static inline uint8_t tcp_get_data_offset(const struct tcp_hdr *tcp_header) {
    return (tcp_header->data_offset >> 4) * 4;  // Data offset field is in 32-bit words
}

// Function to check if a specific TCP flag is set
static inline bool tcp_flag_is_set(uint8_t flags, uint8_t flag) {
    return (flags & flag) != 0;
}

__END_DECLS

#endif  // _TCP_H_

