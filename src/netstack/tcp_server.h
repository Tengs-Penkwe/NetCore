#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <netutil/ip.h>
#include <netutil/tcp.h>
#include <netstack/tcp.h>
#include "tcp_connect.h"

#include <interface/rpc.h>
#include <pthread.h>            // worker thread
#include <semaphore.h>          // semaphore for worker thread
#include <lock_free/bdqueue.h>  // message queue for worker thread 

typedef struct tcp_state  TCP;
typedef struct tcp_server TCP_server;
typedef struct tcp_connection TCP_conn;

#define CUCKOO_TABLE_NAME tcp_conn_table
#define CUCKOO_KEY_TYPE tcp_conn_key_t
#define CUCKOO_MAPPED_TYPE TCP_conn
#include <libcuckoo-c/cuckoo_table_template.h>
#undef CUCKOO_TABLE_NAME
#undef CUCKOO_KEY_TYPE
#undef CUCKOO_MAPPED_TYPE

#define TCP_SERVER_DEFAULT_CONN      64
#define TCP_SERVER_QUEUE_SIZE        128

typedef struct tcp_worker {
    // Each worker has a queue
    alignas(ATOMIC_ISOLATION)
        BdQueue     msg_queue;
    size_t          queue_size;
    TCP_server     *server;
    sem_t           sem;
    pthread_t       self;
} TCP_worker __attribute__((aligned(ATOMIC_ISOLATION)));

typedef struct tcp_server {
    TCP_worker         *workers;
    uint8_t             worker_num;

    // For the closing of the server
    bool                is_live;

    struct tcp_state   *tcp;
    tcp_port_t          port;

    // The process who has this server
    struct rpc         *rpc;            // Send message back to process
    tcp_server_callback callback;       // Triggered when a message is received
    uint32_t            max_conn;       // How many connections allowed ?
    
    tcp_conn_table*     conn_table;     // Connection table
} TCP_server;

#include "kavl-lite.h"          // k-avl tree for single connection
typedef struct tcp_connection {
    struct tcp_server    *server;
    // Who send it
    ip_context_t          src_ip;
    tcp_port_t            src_port;
    // Expect message
    uint32_t              sendno;
    union {
        uint32_t          recvno;
        uint32_t          nextno;
    };
    // State
    TCP_st                state;
    KAVLL_HEAD(TCP_msg)   head;
} TCP_conn;

__BEGIN_DECLS

static inline BdQueue* which_queue(TCP_server* server, ip_context_t ip, tcp_port_t port) {
    ipv6_addr_t ip_addr;
    if (ip.is_ipv6) {
        ip_addr = ip.ipv6;
    } else {
        ip_addr = ipv4_to_ipv6(ip.ipv4);
    }
    size_t index = ((size_t)ip_addr + (size_t)port) % server->worker_num;
    return &server->workers[index].msg_queue;
}

errval_t tcp_server_register(
    TCP* tcp, struct rpc* rpc, const tcp_port_t port, const tcp_server_callback callback, size_t worker_num
);

errval_t tcp_server_deregister(
    TCP* tcp, const tcp_port_t port
);

errval_t server_listen(
    TCP_server* server, int max_conn
);

errval_t server_marshal(
    TCP_server* server, ip_context_t dst_ip, tcp_port_t dst_port, Buffer buf
);

errval_t server_send(
    TCP_server* server, TCP_msg* msg
);

errval_t server_unmarshal(
    TCP_server* server, TCP_msg* msg
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

__END_DECLS

// Function to dump the contents of a TCP_conn struct
void dump_tcp_conn(const TCP_conn *conn);

#endif // __TCP_SERVER_H__