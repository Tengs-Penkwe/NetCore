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
    err = hash_init(&tcp->servers, tcp->buckets, TCP_SERVER_BUCKETS, HS_FAIL_ON_EXIST);
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of tcp servers");

    TCP_NOTE("TCP Module Initialized, the hash-table for server has size %d", TCP_SERVER_BUCKETS);
    return SYS_ERR_OK;
}

errval_t tcp_marshal(
    TCP* tcp, const ip_addr_t dst_ip, const tcp_port_t src_port, const tcp_port_t dst_port,
    uint32_t seqno, uint32_t ackno, uint32_t window, uint16_t urg_prt, uint8_t flags,
    Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(tcp);

    buffer_sub_ptr(&buf, sizeof(struct tcp_hdr));

    uint8_t data_offset = TCPH_SET_LEN(sizeof(struct tcp_hdr));

    struct tcp_hdr* packet = (struct tcp_hdr*)buf.data;
    *packet = (struct tcp_hdr) {
        .src_port    = htons(src_port),
        .dest_port   = htons(dst_port),
        .seqno       = htonl(seqno),
        .ackno       = htonl(ackno),
        .data_offset = data_offset,
        .flags       = flags,
        .window      = htons(window),
        .chksum      = 0,
        .urgent_ptr  = urg_prt,
    };

    struct pseudo_ip_header_in_net_order ip_header = {
        .src_addr       = htonl(tcp->ip->my_ip),
        .dst_addr       = htonl(dst_ip),
        .reserved       = 0,
        .protocol       = IP_PROTO_TCP,
        .len_no_iph     = htonl(buf.valid_size),
    };
    packet->chksum  = tcp_checksum_in_net_order(buf.data, ip_header);

    err = ip_marshal(tcp->ip, dst_ip, IP_PROTO_TCP, buf);
    DEBUG_FAIL_RETURN(err, "Can't marshal the TCP packet and sent by IP");

    return err;
}

errval_t tcp_unmarshal(
    TCP* tcp, const ip_addr_t src_ip, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(tcp);
    struct tcp_hdr* packet = (struct tcp_hdr*)buf.data;

    // 0. Check Validity
    uint8_t reserved = TCP_RSVR(packet);
    if (reserved != 0x00) {
        TCP_ERR("The TCP reserved field 0x%02x should be 0 !", reserved);
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
    struct pseudo_ip_header_in_net_order ip_header = {
        .src_addr       = htons(src_ip),
        .dst_addr       = htons(tcp->ip->my_ip),
        .reserved       = 0,
        .protocol       = IP_PROTO_TCP,
        .len_no_iph     = htons(buf.valid_size),
    };
    uint16_t chksum = ntohs(packet->chksum);
    packet->chksum  = 0;
    uint16_t tcp_chksum = ntohs(tcp_checksum_in_net_order(buf.data, ip_header));
    if (chksum != tcp_chksum) {
        TCP_ERR("The TCP checksum %p should be %p", chksum, tcp_chksum);
        return NET_ERR_TCP_WRONG_FIELD;
    }

    // 2. Create the Message
    uint32_t seqno = ntohl(packet->seqno);
    uint32_t ackno = ntohl(packet->ackno);

    // We ignore it for now
    uint32_t window = ntohs(packet->window);
    (void) window;

    // 3. Create the Message
    uint8_t flags = packet->flags;
    TCP_msg* msg = calloc(1, sizeof(TCP_msg));
    assert(msg);
    *msg = (TCP_msg) {
        .seqno    = seqno,
        .ackno    = ackno,
        .buf      = buffer_add(buf, offset),
        .flags    = get_tcp_flags(flags),
        .recv     = {
            .src_ip   = src_ip,
            .src_port = src_port,
        },
    };
    
    // 4. Find the Server
    TCP_server* server = NULL;

    err = hash_get_by_key(&tcp->servers, TCP_HASH_KEY(dst_port), (void**)&server);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        assert(server);
        USER_PANIC("need to consider the server is deregistered during the process");
        sem_wait(&server->sema);
        if (server->is_live == false)
        {
            sem_post(&server->sema);
            free(msg);
            TCP_ERR("A process try to send message a dead TCP server on this port: %d", dst_port);
            return NET_ERR_TCP_PORT_NOT_REGISTERED;
        }
        else
        {
            // 4.1 If the server is live, then we put the message into the queue
            err = enbdqueue(&server->msg_queue, NULL, (void*)msg);
            if (err_is_fail(err)) {
                assert(err_no(err) == EVENT_ENQUEUE_FULL);
                TCP_ERR("The given message queue of TCP message is full, will drop this message in upper level");
                return err_push(err, NET_ERR_TCP_QUEUE_FULL);
            }
            sem_post(&server->sema);
            return NET_THROW_TCP_ENQUEUE;
        }
        assert(0);
    case EVENT_HASH_NOT_EXIST: 
        TCP_ERR("A process try to send message to a not existing TCP server on this port: %d", dst_port);
        return NET_ERR_TCP_PORT_NOT_REGISTERED;
    default: USER_PANIC_ERR(err, "Unknown Error Code");
    }
}
