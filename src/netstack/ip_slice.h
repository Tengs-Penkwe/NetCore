#ifndef __IP_SLICE_H__
#define __IP_SLICE_H__

#include <stdatomic.h> 
#include <netstack/ip.h>

/// Ethernet Header (14) => round to 8
#define IP_RESERVER_SIZE     16
#define IP_MINIMUM_NO_FRAG   576

/// @brief Presentation of an IP Sending Message 
typedef struct ip_send {
    struct ip_state *ip;     ///< Global IP state
    ip_addr_t        dst_ip;
    mac_addr         dst_mac;

    uint8_t          proto;  ///< Protocal over IP
    uint16_t         id;     ///< Message ID

    Buffer           buf;
    uint16_t         sent_size;   ///< How many bytes have we sent

    int              retry_interval;
} IP_send;

__BEGIN_DECLS

void close_sending_message(void* send);
void check_get_mac(void* message);
void check_send_message(void* message);

errval_t ip_send(
    IP* ip, const ip_addr_t dst_ip, const mac_addr dst_mac,
    const uint16_t id, const uint8_t proto,
    Buffer buf,
    const uint16_t send_from, const uint16_t size_to_send,
    bool last_slice
);

errval_t ip_slice(IP_send* msg);

__END_DECLS

#endif // __IP_SLICE_H__