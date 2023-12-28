#include <netstack/tcp.h>
#include "tcp_server.h"
#include "tcp_connect.h"
#include <event/states.h>

errval_t tcp_server_register(
    TCP* tcp, rpc_t* rpc, const tcp_port_t port, const tcp_server_callback callback
) {
    assert(tcp);
    errval_t err_get, err_insert;

    TCP_server *get_server = NULL; 
    TCP_server *new_server = calloc(1, sizeof(TCP_server));
    *new_server = (TCP_server) {
        .max_worker = g_states.max_workers_for_single_tcp_server,
        .is_live    = true,
        .sema       = { { 0 } },
        .tcp        = tcp,
        .rpc        = rpc,
        .port       = port,
        .callback   = callback,
        .max_conn   = 0,        // Need user to set
    };

    if (sem_init(&new_server->sema, 0, new_server->max_worker) != 0){
        TCP_ERR("Can't initialize the semaphore of TCP server");
        return SYS_ERR_INIT_FAIL;
    }

    //TODO: reconsider the multithread contention here
    err_get = hash_get_by_key(&tcp->servers, TCP_HASH_KEY(port), (void**)&get_server);
    switch (err_no(err_get))
    {
    case SYS_ERR_OK:
    {    // We may meet a dead server, since the hash table is add-only, we can't remove it
        assert(get_server);
        sem_wait(&get_server->sema);

        if (get_server->is_live == false)   // Dead Server
        {
            for (size_t i = 0; i < get_server->max_worker - 1; i++) {
                sem_wait(&get_server->sema);
            }   // Wait until all workers' done with this server
            assert(get_server->tcp  == tcp);
            assert(get_server->port == port);
            assert(get_server->max_worker == 8);
            get_server->rpc      = rpc;
            get_server->callback = callback;
            get_server->is_live  = true;

            for (size_t i = 0; i < get_server->max_worker; i++) {
                sem_post(&get_server->sema);
            }   // Now the new server is ready
            assert(sem_destroy(&new_server->sema) == 0);
            free(new_server);
            return SYS_ERR_OK;
        }
        else    // Live server
        {
            sem_post(&get_server->sema);
            assert(sem_destroy(&new_server->sema) == 0);
            free(new_server);
            return NET_ERR_TCP_PORT_REGISTERED;
        }
        assert(0);
    }
    case EVENT_HASH_NOT_EXIST:
        break;
    default: USER_PANIC_ERR(err_get, "Unknown Error Code");
    }

    assert(err_no(err_get) == EVENT_HASH_NOT_EXIST);

    err_insert = hash_insert(&tcp->servers, TCP_HASH_KEY(port), get_server, false);
    switch (err_no(err_insert)) 
    {
    case SYS_ERR_OK:
        return SYS_ERR_OK;
    case EVENT_HASH_NOT_EXIST:
        assert(sem_destroy(&new_server->sema) == 0);
        free(new_server);
        TCP_ERR("Another process also wants to register the TCP port and he/she gets it")
        return NET_ERR_TCP_PORT_REGISTERED;
    default: USER_PANIC_ERR(err_insert, "Unknown Error Code");
    }
}

errval_t tcp_server_deregister(
    TCP* tcp, const tcp_port_t port
) {
    assert(tcp);
    errval_t err;
    
    TCP_server* server = NULL;
    err = hash_get_by_key(&tcp->servers, TCP_HASH_KEY(port), (void**)&server);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        sem_wait(&server->sema);
        if (server->is_live == false) {
            sem_post(&server->sema);
            TCP_ERR("A process try to de-register a dead TCP server on this port: %d", port);
            return NET_ERR_TCP_PORT_NOT_REGISTERED;
        } else {
            server->is_live = false;
            sem_post(&server->sema);
            TCP_INFO("We inactivated a TCP server at port: %d", port);
            return SYS_ERR_OK;
        }
        assert(0);
    case EVENT_HASH_NOT_EXIST:
        TCP_ERR("A process try to de-register a not existing TCP server on this port: %d", port);
        return NET_ERR_TCP_PORT_NOT_REGISTERED;
    default: USER_PANIC_ERR(err, "Unknown Error Code");
    }
}

static errval_t server_find_or_create_connection(
    TCP_server* server, TCP_msg* msg, TCP_conn** conn
) {
    uint64_t key = TCP_CONN_KEY(msg->recv.src_ip, msg->recv.src_port);
    (void) key;
    USER_PANIC("NYI");

    // if (collections_hash_size(server->connections) > server->max_conn) {
    //     return NET_ERR_TCP_MAX_CONNECTION;
    // }

    // (*conn) = collections_hash_find(server->connections, key);
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
        // collections_hash_insert(server->connections, key, (*conn));
    } 
    assert(*conn);
    return SYS_ERR_OK;
}

// static void free_connection(void* connection) {
//     TCP_conn* conn = connection;
//     LOG_ERR("TODO: cancel deferred event?");
//     free(conn);
//     connection = NULL;
// }

errval_t server_listen(
    TCP_server* server, int max_conn
) {
    assert(server);
    server->max_conn = max_conn > 0 ? max_conn : 0;
    return SYS_ERR_OK;
}

errval_t server_marshal(
    TCP_server* server, ip_addr_t dst_ip, tcp_port_t dst_port, uint8_t* data, uint16_t size
) {
    errval_t err;

    uint64_t key = TCP_CONN_KEY(dst_ip, dst_port);
    (void) key;
    LOG_ERR("NYI");
    TCP_conn* conn = NULL; //collections_hash_find(server->connections, key);
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
    USER_PANIC("NYI");
        // msg->data = malloc(HEADER_RESERVED_SIZE);
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