#include <netutil/udp.h>
#include <netutil/htons.h>
#include <netutil/checksum.h>

#include <netstack/udp.h>
#include <netstack/ip.h>

errval_t udp_init(
    UDP* udp, IP* ip
) {
    assert(udp && ip);
    udp->ip = ip;


    return SYS_ERR_OK;
}

void udp_destroy(
    UDP* udp
) {
    assert(udp);
    UDP_ERR("TODO: free the hash table");
}

errval_t udp_marshal(
    UDP* udp, const ip_addr_t dst_ip, const udp_port_t src_port, const udp_port_t dst_port,
    void* addr, size_t size
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

    // struct pseudo_ip_header ip_header = {
    //     .src_addr = udp->ip->ip,
    //     .dst_addr = dst_ip,
    //     .reserved = 0,
    //     .protocol = IP_PROTO_UDP,
    //     .whole_length = ((uint16_t)size),
    // };
    // packet->chksum = htons(tcp_udp_checksum(addr, size, ip_header));
    //TODO: Calculate the chksum, although it's optional

    err = ip_marshal(udp->ip, dst_ip, IP_PROTO_UDP, addr, size);
    RETURN_ERR_PRINT(err, "Can't marshal the message and sent by IP");

    return SYS_ERR_OK;
}

errval_t udp_unmarshal(
    UDP* udp, const ip_addr_t src_ip, void* addr, size_t size
) {
    assert(udp && addr);
    LOG_DEBUG("Received an UDP packet, source IP: %p, addr:%p, size: %d", src_ip, addr, size);

    struct udp_hdr* packet = (struct udp_hdr*) addr;

    if (ntohs(packet->len) != size) {
        LOG_ERR("UDP Packet Size Unmatch %p v.s. %p", ntohs(packet->len), size);
        return NET_ERR_UDP_WRONG_FIELD;
    }
    assert(size >= UDP_LEN_MIN && size <= UDP_LEN_MAX);

    udp_port_t src_port = ntohs(packet->src);
    udp_port_t dst_port = ntohs(packet->dest);

    // 2. Checksum is optional
    uint16_t pkt_chksum = packet->chksum; //TODO: ntohs ?
    if (pkt_chksum != 0) {
        // packet->chksum = 0;
        // // uint16_t checksum = net_checksum_tcpudp(size, IP_PROTO_UDP, addr);
        // uint16_t checksum = 0;//udp_checksum(addr, size, src_port, dst_port);
        // if (pkt_chksum != ntohs(checksum)) {
        //     LOG_ERR("This UDP Pacekt Has Wrong Checksum %p, Should be %p", checksum, pkt_chksum);
        //     // return NET_ERR_UDP_WRONG_FIELD;
        // }
    }

    UDP_server* server = collections_hash_find(udp->servers, UDP_KEY(dst_port));

    addr += sizeof(struct udp_hdr);
    size -= sizeof(struct udp_hdr);

    if (server == NULL) {
        LOG_ERR("We don't have UDP server on this port: %d", dst_port);
        return NET_ERR_UDP_PORT_NOT_REGISTERED;
    } else {
        server->callback(server, addr, size, src_ip, src_port);
        LOG_DEBUG("We handled an UDP packet at port: %d", dst_port);
    }

    return SYS_ERR_OK;
}

errval_t udp_server_register(
    UDP* udp, int fd, const udp_port_t port, const udp_server_callback callback
) {
    assert(udp);
    UDP_server* server = collections_hash_find(udp->servers, UDP_KEY(port));
    
    if (server != NULL) return NET_ERR_UDP_PORT_REGISTERED;
    server = calloc(1, sizeof(UDP_server));
    assert(server);

    *server = (UDP_server) {
        .udp      = udp,
        .fd       = fd,
        .chan     = rpc,
        .port     = port,
        .callback = callback,
    };

    collections_hash_insert(udp->servers, UDP_KEY(port), server);

    return SYS_ERR_OK;
}

errval_t udp_server_deregister(
    UDP* udp, const udp_port_t port
) {
    assert(udp);
    UDP_server* server = collections_hash_find(udp->servers, UDP_KEY(port));
    
    if (server != NULL) return NET_ERR_UDP_PORT_NOT_REGISTERED;
    collections_hash_delete(udp->servers, UDP_KEY(port));
    LOG_INFO("We deleted a UDP server at port: %d", port);

    return SYS_ERR_OK;
}