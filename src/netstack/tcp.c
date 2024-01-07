#include <netstack/tcp.h>
#include <netstack/ip.h>
#include <netutil/checksum.h>
#include <netutil/htons.h>
#include "tcp_server.h"

errval_t tcp_init(TCP* tcp, IP* ip) {
    errval_t err;
    assert(tcp && ip);
    tcp->ip = ip;

    // 1. Hash table for servers
    err = hash_init(
        &tcp->servers, tcp->buckets, TCP_SERVER_BUCKETS, HS_FAIL_ON_EXIST,
        voidptr_key_cmp, voidptr_key_hash
    );
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of tcp servers");

    TCP_NOTE("TCP Module Initialized, the hash-table for server has size %d", TCP_SERVER_BUCKETS);
    return SYS_ERR_OK;
}

void tcp_destroy(
    TCP* tcp
) {
    assert(tcp);

    // 1. Destroy the hash table for servers
    hash_destroy(&tcp->servers);
    memset(tcp, 0x00, sizeof(TCP));
    free(tcp);

    TCP_ERR("Not implemented yet");
}

errval_t tcp_send(
    TCP* tcp, const ip_context_t dst_ip, const tcp_port_t src_port, const tcp_port_t dst_port,
    uint32_t seqno, uint32_t ackno, uint32_t window, uint16_t urg_ptr, uint8_t flags,
    Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(tcp);

    buffer_sub_ptr(&buf, sizeof(struct tcp_hdr));

    struct tcp_hdr* packet = (struct tcp_hdr*)buf.data;
    *packet = (struct tcp_hdr) {
        .src_port    = htons(src_port),
        .dest_port   = htons(dst_port),
        .seqno       = htonl(seqno),
        .ackno       = htonl(ackno),
        .data_offset = sizeof(struct tcp_hdr) / 4,
        .reserved    = 0,
        .flags       = flags,
        .window      = htons(window),
        .chksum      = 0,
        .urgent_ptr  = urg_ptr,
    };

    struct pseudo_ip_header_in_net_order ip_header;
    if (dst_ip.is_ipv6) 
        ip_header = PSEUDO_HEADER_IPv6(tcp->ip->my_ipv6, dst_ip.ipv6, IP_PROTO_UDP, buf.valid_size);
    else
        ip_header = PSEUDO_HEADER_IPv4(tcp->ip->my_ipv4, dst_ip.ipv4, IP_PROTO_UDP, buf.valid_size);
    packet->chksum  = tcp_checksum_in_net_order(buf.data, ip_header);

    err = ip_marshal(tcp->ip, dst_ip, IP_PROTO_TCP, buf);
    DEBUG_FAIL_RETURN(err, "Can't marshal the TCP packet and sent by IP");

    return err;
}

errval_t tcp_unmarshal(
    TCP* tcp, const ip_context_t src_ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(tcp);
    struct tcp_hdr* packet = (struct tcp_hdr*)buf.data;

    // 0. Check Validity
    if (packet->reserved != 0x00) {
        TCP_ERR("The TCP reserved field 0x%02x should be 0 !", packet->reserved);
        return NET_ERR_TCP_WRONG_FIELD;
    }
    uint8_t offset = TCP_HLEN(packet);
    if (!(offset >= TCP_HLEN_MIN && offset <= TCP_HLEN_MAX)) {
        TCP_ERR("The TCP header size: %d is invalid", offset);
        return NET_ERR_TCP_WRONG_FIELD;
    }

    // 1. Find the Connection
    tcp_port_t src_port = ntohs(packet->src_port);
    tcp_port_t dst_port = ntohs(packet->dest_port);
    
    // 2. Checksum
    struct pseudo_ip_header_in_net_order ip_header;
    if (src_ip.is_ipv6) 
        ip_header = PSEUDO_HEADER_IPv6(tcp->ip->my_ipv6, src_ip.ipv6, IP_PROTO_TCP, (uint32_t)buf.valid_size);
    else
        ip_header = PSEUDO_HEADER_IPv4(tcp->ip->my_ipv4, src_ip.ipv4, IP_PROTO_TCP, (uint16_t)buf.valid_size);
    uint16_t chksum = ntohs(packet->chksum);
    packet->chksum  = 0;
    uint16_t tcp_chksum = ntohs(tcp_checksum_in_net_order(buf.data, ip_header));
    if (chksum != tcp_chksum) {
        TCP_ERR("The TCP checksum %p should be %p", chksum, tcp_chksum);
        return NET_ERR_TCP_WRONG_FIELD;
    }

    // 3. Create the Message
    uint8_t flags = packet->flags;
    TCP_msg* msg = calloc(1, sizeof(TCP_msg));
    assert(msg);
    *msg = (TCP_msg) {
        .recv     = {
            .src_ip   = src_ip,
            .src_port = src_port,
        },
        .flags    = get_tcp_flags(flags),
        .seqno    = ntohl(packet->seqno),
        .ackno    = ntohl(packet->ackno),
        .window   = ntohs(packet->window),
        .urg_ptr  = ntohs(packet->urgent_ptr),
        .buf      = buffer_add(buf, offset),
    };
    
    // 4. Find the Server
    TCP_server* server = NULL;

    err = hash_get_by_key(&tcp->servers, TCP_HASH_KEY(dst_port), (void**)&server);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        assert(server);
        USER_PANIC("need to consider the server is deregistered during the process");
        if (server->is_live == false)
        {
            free(msg);
            TCP_ERR("A process try to send message a dead TCP server on this port: %d", dst_port);
            return NET_ERR_TCP_PORT_NOT_REGISTERED;
        }
        else
        {
            BdQueue* queue = which_queue(server, src_ip, src_port);
            // 4.1 If the server is live, then we put the message into the queue
            err = enbdqueue(queue, NULL, (void*)msg);
            if (err_is_fail(err)) {
                assert(err_no(err) == EVENT_ENQUEUE_FULL);
                TCP_ERR("The given message queue of TCP message is full, will drop this message in upper level");
                return err_push(err, NET_ERR_TCP_QUEUE_FULL);
            }
            return NET_THROW_TCP_ENQUEUE;
        }
        assert(0);
    case EVENT_HASH_NOT_EXIST: 
        TCP_ERR("A process try to send message to a not existing TCP server on this port: %d", dst_port);
        return NET_ERR_TCP_PORT_NOT_REGISTERED;
    default: USER_PANIC_ERR(err, "Unknown Error Code");
    }
}
