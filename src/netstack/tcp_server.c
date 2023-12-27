#include <netstack/tcp.h>
#include "tcp_server.h"
#include "tcp_connect.h"

errval_t tcp_server_register(
    TCP* tcp, struct rpc* rpc, const tcp_port_t port, const tcp_server_callback callback
) {
    assert(tcp);
    errval_t err;

    TCP_server* server = collections_hash_find(tcp->servers, TCP_HASH_KEY(port));
    if (server != NULL) return NET_ERR_TCP_PORT_REGISTERED;

    server = calloc(1, sizeof(TCP_server));
    assert(server);
    
    err = server_init(tcp, server, rpc, port, callback);
    RETURN_ERR_PRINT(err, "Can't initialize the server");

    collections_hash_insert(tcp->servers, (uint64_t)port, server);

    return SYS_ERR_OK;
}

errval_t tcp_server_deregister(
    TCP* tcp, const tcp_port_t port
) {
    assert(tcp);
    TCP_server* server = collections_hash_find(tcp->servers, TCP_HASH_KEY(port));
   
    if (server != NULL) return NET_ERR_UDP_PORT_NOT_REGISTERED;
    collections_hash_delete(tcp->servers, (uint64_t)port);
    LOG_INFO("We deleted a TCP server at port: %d", port);

    return SYS_ERR_OK;
}

static errval_t server_find_or_create_connection(
    TCP_server* server, TCP_msg* msg, TCP_conn** conn
) {
    uint64_t key = TCP_CONN_KEY(msg->recv.src_ip, msg->recv.src_port);

    if (collections_hash_size(server->connections) > server->max_conn) {
        return NET_ERR_TCP_MAX_CONNECTION;
    }

    (*conn) = collections_hash_find(server->connections, key);
    if ((*conn) == NULL) {
        (*conn) = calloc(1, sizeof(TCP_conn));
        assert((*conn));
        if (msg->flags != TCP_FLAG_SYN) {
            return NET_ERR_TCP_NO_CONNECTION;
        }
        *(*conn) = (TCP_conn) {
            .server    = server,
            .src_ip    = msg->recv.src_ip,
            .src_port  = msg->recv.src_port,
            .sendno    = msg->seqno,
            .recvno    = msg->ackno,
            .state     = LISTEN,
        };
        collections_hash_insert(server->connections, key, (*conn));
    } 
    assert(*conn);
    return SYS_ERR_OK;
}

static void free_connection(void* connection) {
    TCP_conn* conn = connection;
    LOG_ERR("TODO: cancel deferred event?");
    free(conn);
    connection = NULL;
}

void server_shutdown(
    TCP_server* server
) {
    assert(server);
    collections_hash_release(server->connections);
}

errval_t server_init(
    TCP* tcp, TCP_server* server, int fd, struct aos_rpc* rpc, const tcp_port_t port, const tcp_server_callback callback
) {
    assert(tcp && server);
    *server = (TCP_server) {
        .tcp      = tcp,
        .fd       = fd,
        .chan     = rpc,
        .port     = port,
        .callback = callback,
        .max_conn  = 0,       // Server must use listen() to specify this
    };    

    // Create the connection list
    collections_hash_create_with_buckets(&server->connections, TCP_DEFAULT_CONN, free_connection);
    return SYS_ERR_OK;
}

errval_t server_listen(
    TCP_server* server, int max_conn
) {
    assert(server);
    server->max_conn = max_conn > 0 ? max_conn : 0;
    return SYS_ERR_OK;
}

errval_t server_marshal(
    TCP_server* server, ip_addr_t dst_ip, tcp_port_t dst_port, void* data, size_t size
) {
    errval_t err;

    uint64_t key = TCP_CONN_KEY(dst_ip, dst_port);
    TCP_conn* conn = collections_hash_find(server->connections, key);
    if (conn == NULL) {
        return NET_ERR_TCP_NO_CONNECTION;
    }

    TCP_msg send_msg = {
        .send = {
            .dst_ip   = conn->src_ip,
            .dst_port = conn->src_port,
        },
        .flags = TCP_FLAG_ACK,
        .seqno = conn->sendno,
        .ackno = conn->recvno,
        .data  = data,
        .size  = size,
    };
    conn->sendno += size;

    err = server_send(server, &send_msg);
    RETURN_ERR_PRINT(err, "Can't send the TCP msg");

    //TODO: we don't need to free the data, IP level will do it;
    return SYS_ERR_OK;
}

errval_t server_send(
    TCP_server* server, TCP_msg* msg
) {
    errval_t err;
    if (msg->size == 0) {
        assert(msg->data == NULL);
        msg->data = malloc(HEADER_RESERVED_SIZE);
    }
    assert(server && msg->data);

    LOG_ERR("Decide window and urg_ptr!");
    uint16_t window = 65535;
    uint16_t urg_ptr = 0;
    uint8_t flags = flags_compile(msg->flags);

    err = tcp_marshal(
        server->tcp, msg->send.dst_ip, server->port, msg->send.dst_port, 
        msg->seqno, msg->ackno, window, urg_ptr, flags,
        msg->data, msg->size
    );
    RETURN_ERR_PRINT(err, "Can't marshal this tcp message !");

    return SYS_ERR_OK;
}

errval_t server_unmarshal(
    TCP_server* server, TCP_msg* msg
) {
    assert(server && msg);
    errval_t err;

    TCP_conn* conn = NULL;
    err = server_find_or_create_connection(server, msg, &conn);
    RETURN_ERR_PRINT(err, "Can't find the Connection for this TCP message !"); 

    /// TODO: We can't assume the message arrives in order, we must do something here
    LOG_ERR("Can't Assume the order of the message !");

    /// Dangerous, we should make sure the order is OK !
    err = conn_handle_msg(conn, msg);
    RETURN_ERR_PRINT(err, "Can't handle this message !");

    return SYS_ERR_OK;
}


////////////////////////////////////////////////////////////////////////////
/// Dump functions
////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

void dump_tcp_conn(const TCP_conn *conn) {
    char src_ip_str[INET_ADDRSTRLEN];
    ip_addr_t src_ip = htonl(conn->src_ip);
    inet_ntop(AF_INET, &src_ip, src_ip_str, INET_ADDRSTRLEN);

    printf("TCP Connection:\n");
    printf("   Server Pointer: %p\n", (void *)conn->server);
    printf("   Source IP: %s\n", src_ip_str);
    printf("   Source Port: %u\n", (conn->src_port));
    printf("   Send Number: %u\n", (conn->sendno));
    printf("   Receive Number: %u\n", (conn->recvno));
    printf("   State: %s\n", tcp_state_to_string(conn->state));

    // Additional information from `defer` can be printed here if relevant
}