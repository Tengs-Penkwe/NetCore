#include <netstack/tcp.h>
#include "tcp_server.h"
#include "tcp_connect.h"
#include <event/states.h>
#include <sys/syscall.h>   // syscall
#include <netutil/dump.h>  // format_ip_addr
#include <stdatomic.h>     // atomic_thread_fence
#include <unistd.h>        // For usleep

static void* worker_thread(void* localstate);
static void server_destroy(TCP_server* server);

static void* worker_thread(void* localstate) {
    assert(localstate);
    LocalState* local = (LocalState*)localstate;
    local->my_pid = syscall(SYS_gettid);
    TCP_worker* worker = local->my_state;
    TCP_server* server = worker->server;

    TCP_NOTE("%s is running on thread %d", local->my_name, (int)local->my_pid);
    
    CORES_SYNC_BARRIER;
    
    TCP_msg* msg = NULL;
    
    while (true) {
        if (debdqueue(&worker->msg_queue, NULL, (void**)&msg) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&worker->sem);
        } else {
            assert(msg);
            server_unmarshal(server, msg);
            free(msg);
            msg = NULL;
        }
    }
}

static errval_t worker_init(TCP_worker* worker, TCP_server* server, size_t queue_size, size_t worker_id) {
    assert(worker && server); errval_t err = SYS_ERR_OK;

    worker->server = server;

    worker->queue_size = queue_size;
    BQelem * queue_elements = calloc(worker->queue_size, sizeof(BQelem));
    err = bdqueue_init(&worker->msg_queue, queue_elements, worker->queue_size);
    if (err_is_fail(err)) {
        TCP_FATAL("Can't Initialize the queues of TCP messages for worker");
        goto clean_queue_init;
    }
    
    if (sem_init(&worker->sem, 0, 0) != 0){
        TCP_FATAL("Can't initialize the semaphore of TCP Worker");
        goto clean_sem_init;
    }

    LocalState* local = calloc(1, sizeof(LocalState));
    // Now we have a new server, we need to start a new thread(s) for it
    char* name = calloc(32, sizeof(char));
    sprintf(name, "TCP(%d)Server%d", server->port, (int)worker_id);

    *local = (LocalState) {
        .my_name  = name,
        .my_pid   = (pid_t)-1,   // Don't know yet
        .log_file = (g_states.log_file == NULL) ? stdout : g_states.log_file,
        .my_state = worker,
    };

    if (pthread_create(&worker->self, NULL, worker_thread, (void*)local) != 0) {
        TCP_FATAL("Can't create worker thread");
        goto clean_thread_create;
    }
    return err;

clean_thread_create:
    err = err_push(err, NET_ERR_TCP_CREATE_WORKER);
    free(local);
clean_sem_init:
    bool queue_elements_from_heap = true;
    bdqueue_destroy(&worker->msg_queue, queue_elements_from_heap);
clean_queue_init:
    free(queue_elements);
    return err_push(err, SYS_ERR_INIT_FAIL);
}

static void worker_destroy(TCP_worker* worker) {
    assert(worker);
    bool queue_elements_from_heap = true;
    bdqueue_destroy(&worker->msg_queue, queue_elements_from_heap);
    sem_destroy(&worker->sem);
    pthread_cancel(worker->self);
    TCP_ERR("Free the name of worker");
    free(worker);
}

static errval_t server_init(TCP_server* server) {
    errval_t err = SYS_ERR_OK;

    server->conn_table = tcp_conn_table_init(TCP_SERVER_DEFAULT_CONN);

    int i = 0;
    for (; i < server->worker_num; i++) {
        TCP_worker* worker = aligned_alloc(ATOMIC_ISOLATION, sizeof(TCP_worker));
        memset(worker, 0x00, sizeof(TCP_worker));

        err = worker_init(worker, server, TCP_SERVER_QUEUE_SIZE, (size_t)i);
        if (err_is_fail(err)) {
            TCP_FATAL("Can't initialize the worker for TCP server");
            goto clean_worker_init;
        }
    }
    return err;

clean_worker_init:
    for (i-- ; i >= 0; i--) {
        worker_destroy(&server->workers[i]);
    }
    return err_push(err, SYS_ERR_INIT_FAIL);
}

static void server_destroy(TCP_server* server) {
    assert(server->is_live == false);

    for(size_t i = 0; i < server->worker_num; i++) {
        worker_destroy(&server->workers[i]);
    }
    // Can't free the server, since we may have a dead server in the hash table

    TCP_FATAL("NYI, server_destroy");
}

errval_t tcp_server_register(
    TCP* tcp, rpc_t* rpc, const tcp_port_t port, const tcp_server_callback callback, size_t worker_num
) {
    assert(tcp);
    errval_t err_get, err_insert, err_create;

    TCP_server *get_server = NULL; 
    TCP_server *new_server = aligned_alloc(ATOMIC_ISOLATION, sizeof(TCP_server));
    memset(new_server, 0x00, sizeof(TCP_server));
    *new_server = (TCP_server) {
        .worker_num = worker_num, 
        .is_live    = true,
        .tcp        = tcp,
        .rpc        = rpc,
        .port       = port,
        .callback   = callback,
        .max_conn   = 0,       // Need user to set
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

    err_insert = hash_insert(&tcp->servers, TCP_HASH_KEY(port), get_server);
    switch (err_no(err_insert)) 
    {
    case SYS_ERR_OK:
        TCP_NOTE("We registered a TCP server at port: %d", port);
        break;
    case EVENT_HASH_EXIST_ON_INSERT:
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
            atomic_thread_fence(memory_order_release); 
            usleep(10000); // This is unsafe, we should use something like Hazard Pointer
            server_destroy(server);
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

// TODO: Change it back to TCP_server, we use TCP_worker here since we don't have deletable hash table yet
static errval_t find_or_create_connection(
    TCP_server* server, TCP_msg* msg, TCP_conn** conn
) {
    assert(server && msg && conn);
    errval_t err = SYS_ERR_OK;

    // // if (collections_hash_size(server->connections) > server->max_conn) {
    // //     return NET_ERR_TCP_MAX_CONNECTION;
    // // }

    tcp_conn_key_t conn_key_struct = tcp_conn_key_struct(msg->recv.src_ip, msg->recv.src_port);
    if (tcp_conn_table_find(server->conn_table, &conn_key_struct, *conn)) 
    {
        assert(*conn);
    }
    else
    {
        (*conn) = calloc(1, sizeof(TCP_conn)); assert(*conn);
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
        assert(tcp_conn_table_insert(server->conn_table, &conn_key_struct, *conn));
    }
    return err;
}

// static void free_connection(void* connection) {
//     TCP_conn* conn = connection;
//     TCP_ERR("TODO: cancel deferred event?");
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
    TCP_FATAL("NYI");
    TCP_conn* conn = NULL; //collections_hash_find(server->connections, key);
    if (conn == NULL) {
        return NET_ERR_TCP_NO_CONNECTION;
    }

    TCP_msg send_msg = {
        .send    = {
            .dst_ip   = conn->src_ip,
            .dst_port = conn->src_port,
        },
        .flags   = TCP_FLAG_ACK,
        .seqno   = conn->sendno,
        .ackno   = conn->recvno,
        .window  = 65535,
        .urg_ptr = 0,
        .buf     = buf,
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

    TCP_ERR("Decide window and urg_ptr!");

    err = tcp_send(
        server->tcp, msg->send.dst_ip, server->port, msg->send.dst_port, 
        msg->seqno, msg->ackno, msg->window, msg->urg_ptr, flags_compile(msg->flags), msg->buf
    );
    DEBUG_FAIL_RETURN(err, "Can't marshal this tcp message !");

    return SYS_ERR_OK;
}

// TODO: Change it back to TCP_server, we use TCP_worker here since we don't have deletable hash table yet
errval_t server_unmarshal(
    TCP_server* server, TCP_msg* msg
) {
    assert(server && msg);
    errval_t err = SYS_ERR_OK;

    TCP_conn* conn = NULL;
    err = find_or_create_connection(server, msg, &conn);
    DEBUG_FAIL_RETURN(err, "Can't find the Connection for this TCP message !"); 

    /// TODO: We can't assume the message arrives in order, we must do something here
    /// Dangerous, we should make sure the order is OK !
    if (conn->nextno != msg->seqno) {
        TCP_ERR("Need to detect if the packet arrives in order!");
    }

    err = conn_handle_msg(conn, msg);
    DEBUG_FAIL_RETURN(err, "Can't handle this message !");

    return err;
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