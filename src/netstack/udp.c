#include <netutil/udp.h>
#include <netutil/htons.h>
#include <netutil/checksum.h>

#include <netstack/udp.h>
#include <netstack/ip.h>

#include "udp_server.h"

errval_t udp_init(
    UDP* udp, IP* ip
) {
    errval_t err;
    assert(udp && ip);
    udp->ip = ip;

    err = hash_init(&udp->servers, udp->buckets, UDP_DEFAULT_SERVER, HS_OVERWRITE_ON_EXIST);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the hash table of UDP servers");

    return SYS_ERR_OK;
}

void udp_destroy(
    UDP* udp
) {
    assert(udp);
    hash_destroy(&udp->servers);
    memset(udp, 0x00, sizeof(UDP));
    free(udp);
    UDP_NOTE("UDP Module destroyed !");
}

errval_t udp_marshal(
    UDP* udp, const ip_addr_t dst_ip, const udp_port_t src_port, const udp_port_t dst_port,
    uint8_t* addr, uint16_t size
) {
    errval_t err;
    assert(udp && addr);

    // We left space for the IP header, so we don't need another memcpy in the ip_marshal()
    size += sizeof(struct udp_hdr);
    addr -= sizeof(struct udp_hdr);

    struct udp_hdr* packet = (struct udp_hdr*) addr;
    *packet = (struct udp_hdr) {
        .src    = htons(src_port),
        .dest   = htons(dst_port),
        .len    = htons(size),
        .chksum = 0,
    };

    struct pseudo_ip_header_in_net_order ip_header = {
        .src_addr   = htonl(udp->ip->ip),
        .dst_addr   = htonl(dst_ip),
        .reserved   = 0,
        .protocol   = IP_PROTO_UDP,
        .len_no_iph = htons(size),
    };
    packet->chksum = tcp_udp_checksum_in_net_order(addr, ip_header);

    err = ip_marshal(udp->ip, dst_ip, IP_PROTO_UDP, addr, size);
    RETURN_ERR_PRINT(err, "Can't marshal the message and sent by IP");

    return SYS_ERR_OK;
}

errval_t udp_unmarshal(
    UDP* udp, const ip_addr_t src_ip, uint8_t* addr, uint16_t size
) {
    errval_t err;
    assert(udp && addr);
    UDP_DEBUG("Received an UDP packet, source IP: %p, addr:%p, size: %d", src_ip, addr, size);

    struct udp_hdr* packet = (struct udp_hdr*) addr;

    if (ntohs(packet->len) != size) {
        UDP_ERR("UDP Packet Size Unmatch %p v.s. %p", ntohs(packet->len), size);
        return NET_ERR_UDP_WRONG_FIELD;
    }
    assert(size >= UDP_LEN_MIN && size <= UDP_LEN_MAX);
    if (size >= ETHER_MTU - 20) {
        UDP_NOTE("This UDP paceket is very big: %d", size);
    }

    udp_port_t src_port = ntohs(packet->src);
    udp_port_t dst_port = ntohs(packet->dest);

    // 2. Checksum is optional
    uint16_t pkt_chksum = ntohs(packet->chksum); //TODO: ntohs ?
    if (pkt_chksum != 0) {
        packet->chksum = 0;
        struct pseudo_ip_header_in_net_order ip_header = {
            .src_addr   = htonl(src_ip),
            .dst_addr   = htonl(udp->ip->ip),
            .reserved   = 0,
            .protocol   = IP_PROTO_UDP,
            .len_no_iph = htons(size),
        };
        uint16_t checksum = ntohs(tcp_udp_checksum_in_net_order(addr, ip_header));
        if (pkt_chksum != checksum) {
            UDP_ERR("This UDP Pacekt Has Wrong Checksum 0x%0.4x, Should be 0x%0.4x", pkt_chksum, checksum);
            return NET_ERR_UDP_WRONG_FIELD;
        }
    }

    addr += sizeof(struct udp_hdr);
    size -= sizeof(struct udp_hdr);

    UDP_server* server = NULL;
    err = hash_get_by_key(&udp->servers, UDP_HASH_KEY(dst_port), (void**)&server);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        sem_wait(&server->sema);
        if (server->is_live == false) 
        {
            sem_post(&server->sema);
            UDP_ERR("We received packet for a dead UDP server on this port: %d", dst_port);
            return NET_ERR_UDP_PORT_NOT_REGISTERED;
        } 
        else
        {
            // TODO: the semaphore is to prevent this situation:
            server->callback(server, addr, size, src_ip, src_port);
            sem_post(&server->sema);
            UDP_DEBUG("We handled an UDP packet at port: %d", dst_port);
            return SYS_ERR_OK;
        }
        assert(0);
    case EVENT_HASH_NOT_EXIST:
        UDP_ERR("We don't have UDP server on this port: %d", dst_port);
        return NET_ERR_UDP_PORT_NOT_REGISTERED;
        // return SYS_ERR_OK;
    default:
        DEBUG_ERR(err, "Unknown Error Code");
        return err;
    }
}
