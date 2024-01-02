#include <netstack/tcp.h>
#include "tcp_server.h"
#include "tcp_connect.h"
#include <event/states.h>
#include <sys/syscall.h>   //syscall
#include <netutil/dump.h>  //format_ip_addr
#include <stdatomic.h>      // atomic_thread_fence
#include <unistd.h>         // For usleep
                           
static void* server_thread(void* localstate);
static void server_destroy(TCP_server* server);

static void* server_thread(void* localstate) {
    assert(localstate);
    LocalState* local = (LocalState*)localstate;
    local->my_pid = syscall(SYS_gettid);
    
    CORES_SYNC_BARRIER;

    TCP_server* server = local->my_state;
    
    TCP_msg* msg = NULL;
    
    while (true) {
        if (debdqueue(&server->msg_queue, NULL, (void**)&msg) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&server->worker_sem);
        } else {
            assert(msg);
            server_unmarshal(server, msg);
            free(msg);
            msg = NULL;
        }
    }
}

static errval_t server_init(TCP_server* server) {
    errval_t err = SYS_ERR_OK;
    
    // 1. Message Queue for single-thread handling of TCP
    server->queue_size = TCP_SERVER_QUEUE_SIZE;
    BQelem * queue_elements = calloc(server->queue_size, sizeof(BQelem));
    err = bdqueue_init(&server->msg_queue, queue_elements, server->queue_size);
    if (err_is_fail(err)) {
        free(queue_elements);
        TCP_FATAL("Can't Initialize the queues for TCP messages");
        return err;
    }
    
    if (sem_init(&server->worker_sem, 0, server->worker_num) != 0){
        TCP_ERR("Can't initialize the semaphore of TCP server");
        return SYS_ERR_INIT_FAIL;
    }

    LocalState* local = calloc(server->worker_num, sizeof(LocalState));
    // Now we have a new server, we need to start a new thread(s) for it
    for (size_t i = 0; i < server->worker_num; i++) {
        char* name = calloc(16, sizeof(char));
        sprintf(name, "TCP Server %d", (int)i);

        local[i] = (LocalState) {
            .my_name  = name,
            .my_pid   = (pid_t)-1,      // Don't know yet
            .log_file = (g_states.log_file == NULL) ? stdout : g_states.log_file,
            .my_state = server,     // Provide message queue
        };

        if (pthread_create(&server->worker[i], NULL, server_thread, (void*)&local[i]) != 0) {
            TCP_FATAL("Can't create worker thread");
            bool queue_elements_from_heap = true;
            bdqueue_destroy(&server->msg_queue, queue_elements_from_heap);    
            free(local);
            free(server);
            return NET_ERR_TCP_CREATE_WORKER;
        }
    }
    return err;
}

static void server_destroy(TCP_server* server) {
    assert(server->is_live == false);
    bool queue_elements_from_heap = true;
    bdqueue_destroy(&server->msg_queue, queue_elements_from_heap);

    for(size_t i = 0; i < server->worker_num; i++) {
        pthread_cancel(server->worker[i]);
    }
    LOG_FATAL("NYI, server_destroy");

}

errval_t tcp_server_register(
    TCP* tcp, rpc_t* rpc, const tcp_port_t port, const tcp_server_callback callback
) {
    assert(tcp);
    errval_t err_get, err_insert, err_create;

    TCP_server *get_server = NULL; 
    TCP_server *new_server = aligned_alloc(ATOMIC_ISOLATION, sizeof(TCP_server));
    memset(new_server, 0x00, sizeof(TCP_server));
    *new_server = (TCP_server) {
        .worker_num = g_states.max_workers_for_single_tcp_server,
        .is_live    = true,
        .tcp        = tcp,
        .rpc        = rpc,
        .port       = port,
        .callback   = callback,
        .max_conn   = 0,        // Need user to set
    };

    //TODO: reconsider the multithread contention here
    err_get = hash_get_by_key(&tcp->servers, TCP_HASH_KEY(port), (void**)&get_server);
    switch (err_no(err_get))
    {
    case SYS_ERR_OK:
    {    // We may meet a dead server, since the hash table is add-only, we can't remove it
        assert(get_server);
        if (get_server->is_live == false)   // Dead Server
        {
            server_destroy(get_server);
            *get_server = *new_server;
            free(new_server);
            return SYS_ERR_OK;
        }
        else    // Live server
        {
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
        TCP_NOTE("We registered a TCP server at port: %d", port);
        break;
    case EVENT_HASH_NOT_EXIST:
        free(new_server);
        TCP_ERR("Another process also wants to register the TCP port and he/she gets it")
        return NET_ERR_TCP_PORT_REGISTERED;
    default: USER_PANIC_ERR(err_insert, "Unknown Error Code");
    }
    
    err_create = server_init(new_server);
    DEBUG_FAIL_PUSH(err_create, SYS_ERR_INIT_FAIL, "Can't initialize the TCP server");
    return err_create;
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
        if (server->is_live == false)
        {
            TCP_ERR("A process try to de-register a dead TCP server on this port: %d", port);
            return NET_ERR_TCP_PORT_NOT_REGISTERED;
        }
        else
        {
            server->is_live = false;
            atomic_thread_fence(memory_order_seq_cst); // Apply a sequentially-consistent memory barrier
            usleep(10000); // Sleep for (0.01 seconds) to ensure all the threads have finished their work
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
    // uint64_t key = TCP_CONN_KEY(msg->recv.src_ip, msg->recv.src_port);
    // (void) key;
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
    TCP_server* server, ip_context_t dst_ip, tcp_port_t dst_port, Buffer buf
) {
    errval_t err;

    (void) dst_ip;
    (void) dst_port;
    // uint64_t key = TCP_CONN_KEY(dst_ip, dst_port);
    // (void) key;
    LOG_FATAL("NYI");
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
        .buf   = buf,
    };
    conn->sendno += buf.valid_size;

    err = server_send(server, &send_msg);
    DEBUG_FAIL_RETURN(err, "Can't send the TCP msg");

    //TODO: we don't need to free the data, IP level will do it;
    return SYS_ERR_OK;
}

errval_t server_send(
    TCP_server* server, TCP_msg* msg
) {
    errval_t err;
    if (msg->buf.valid_size == 0) {
        assert(msg->buf.data == NULL);
    USER_PANIC("NYI");
        // msg->data = malloc(HEADER_RESERVED_SIZE);
    }
    assert(server);

    LOG_ERR("Decide window and urg_ptr!");
    uint16_t window = 65535;
    uint16_t urg_ptr = 0;
    uint8_t flags = flags_compile(msg->flags);

    err = tcp_send(
        server->tcp, msg->send.dst_ip, server->port, msg->send.dst_port, 
        msg->seqno, msg->ackno, window, urg_ptr, flags, msg->buf
    );
    DEBUG_FAIL_RETURN(err, "Can't marshal this tcp message !");

    return SYS_ERR_OK;
}

errval_t server_unmarshal(
    TCP_server* server, TCP_msg* msg
) {
    assert(server && msg);
    errval_t err;

    TCP_conn* conn = NULL;
    err = server_find_or_create_connection(server, msg, &conn);
    DEBUG_FAIL_RETURN(err, "Can't find the Connection for this TCP message !"); 

    /// TODO: We can't assume the message arrives in order, we must do something here
    LOG_ERR("Can't Assume the order of the message !");

    /// Dangerous, we should make sure the order is OK !
    err = conn_handle_msg(conn, msg);
    DEBUG_FAIL_RETURN(err, "Can't handle this message !");

    return SYS_ERR_OK;
}


////////////////////////////////////////////////////////////////////////////
/// Dump functions
////////////////////////////////////////////////////////////////////////////


void dump_tcp_conn(const TCP_conn *conn) {
    char src_ip_str[IPv6_ADDRESTRLEN];
    format_ip_addr(conn->src_ip, src_ip_str, sizeof(src_ip_str));

    printf("TCP Connection:\n");
    printf("   Server Pointer: %p\n", (void *)conn->server);
    printf("   Source IP: %s\n", src_ip_str);
    printf("   Source Port: %u\n", (conn->src_port));
    printf("   Send Number: %u\n", (conn->sendno));
    printf("   Receive Number: %u\n", (conn->recvno));
    printf("   State: %s\n", tcp_state_to_string(conn->state));

    // Additional information from `defer` can be printed here if relevant
}