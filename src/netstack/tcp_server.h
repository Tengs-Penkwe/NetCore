#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <netutil/tcp.h>
#include <netstack/tcp.h>
#include "tcp_connect.h"

typedef struct tcp_state TCP;
typedef struct tcp_server TCP_server;

typedef struct tcp_connection {
    struct tcp_server    *server;
    // Who send it
    ip_addr_t             src_ip;
    tcp_port_t            src_port;
    // Expect message
    uint32_t              sendno;
    union {
        uint32_t          recvno;
        uint32_t          nextno;
    };
    // State
    TCP_st                state;

    // struct deferred_event defer;
} TCP_conn;

/// The key of a connection inside the hash table of a server
#define TCP_CONN_KEY(ip, port) ( ((uint64_t)ip << 16) |  ((uint64_t)port) )

typedef struct tcp_server {
    struct tcp_state      *tcp;

    // The process who has this server
    int                    fd;
    struct aos_rpc        *chan;       // Send message back to process
    tcp_server_callback    callback;   // Triggered when a message is received

    // Server state
    uint32_t               max_conn;    // How many connections allowed ?

    // Information about this server
    tcp_port_t             port;
    // collections_hash_table *connections;  // All the messages it holds
} TCP_server;

errval_t server_init(
    TCP* tcp, TCP_server* server, int fd, struct aos_rpc* rpc, tcp_port_t my_port, tcp_server_callback callback
);

void server_shutdown(
    TCP_server* server
);

errval_t server_listen(
    TCP_server* server, int max_conn
);

errval_t server_marshal(
    TCP_server* server, ip_addr_t dst_ip, tcp_port_t dst_port, void* data, size_t size
);

errval_t server_send(
    TCP_server* server, TCP_msg* msg
);

errval_t server_unmarshal(
    TCP_server* server, TCP_msg* msg
);

errval_t server_close(
    TCP_server* server
);

// Function to convert TCP_st enum to a string
__attribute_maybe_unused__
static const char* tcp_state_to_string(TCP_st state) {
    switch (state) {
        case LISTEN: return "LISTEN";
        case SYN_SENT: return "SYN_SENT";
        case SYN_RECVD: return "SYN_RECVD";
        case ESTABLISHED: return "ESTABLISHED";
        case FIN_WAIT_1: return "FIN_WAIT_1";
        case FIN_WAIT_2: return "FIN_WAIT_2";
        case CLOSE_WAIT: return "CLOSE_WAIT";
        case CLOSING: return "CLOSING";
        case LAST_ACK: return "LAST_ACK";
        case CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

// Function to dump the contents of a TCP_conn struct
void dump_tcp_conn(const TCP_conn *conn);

#endif // __TCP_SERVER_H__