#include <netstack/tcp.h>
#include "tcp_server.h"
#include "tcp_connect.h"
#include <netutil/dump.h>

static errval_t server_callback(TCP_conn* conn, Buffer buf)
{
    assert(conn);
    TCP_server* server = conn->server;
    assert(server);
    
    server->callback(server, buf, conn->src_ip, conn->src_port);
    // LOG_DEBUG("We handled an UDP packet at port: %d", conn->);
    return SYS_ERR_OK;
}

static void free_message(void* message) {
    TCP_msg* msg = message;
    free_buffer(msg->buf);
    free(msg);
    message = NULL;
}

/// @brief    Manage the Establishment of the connection in server side
/// @return   NET_ERR_TCP_BAD_STATE to indicate bad state, caller need to call conn_abrupt_close()
///           other error code will be hand back
static errval_t server_handshake_open(
    TCP_conn* conn, TCP_msg* msg
) {
    errval_t err;
    assert(conn && msg);

    switch (conn->state) {
    case LISTEN:
        switch (msg->flags) {
        case TCP_FLAG_SYN:

            assert(msg->ackno == 0);
            assert(msg->buf.valid_size == 0);

            conn->state = SYN_RECVD;
            conn->recvno = msg->seqno;
            conn->sendno = (uint32_t)rand();

            TCP_msg ret_msg = {
                .send = {
                    .dst_ip   = conn->src_ip,
                    .dst_port = conn->src_port,
                },
                .flags = TCP_FLAG_SYN_ACK,
                .seqno = conn->sendno,
                .ackno = conn->recvno + 1,
                .window = 65535,
                .urg_ptr = 0,
                .buf   = NULL_BUFFER,
            };

            err = server_send(conn->server, &ret_msg);
            DEBUG_FAIL_RETURN(err, "Can't marshal this message by server !");
            break;
        default: return NET_ERR_TCP_BAD_STATE;
        }
        break;
    case SYN_RECVD:
        switch (msg->flags) {
        case TCP_FLAG_SYN: 
            USER_PANIC("The Client didn't receive our SYN_ACK");
            break;
        case TCP_FLAG_ACK:

            assert(msg->seqno == conn->recvno + 1);
            assert(msg->ackno == conn->sendno + 1);
            assert(msg->buf.valid_size == 0);

            conn->state  = ESTABLISHED;
            conn->sendno += 1;
            conn->recvno += 1;
            break;
        case TCP_FLAG_FIN:
        default: return NET_ERR_TCP_BAD_STATE;
        }
        break;
    default: return NET_ERR_TCP_BAD_STATE;
    }
    return SYS_ERR_OK;
}

// static errval_t client_handshake_open(

// )

/// @brief   Manage the Data Exchange in both server and client
/// @return  NET_ERR_TCP_BAD_STATE to indicate bad state, caller need to call conn_abrupt_close()
///          other error code will be hand back
static errval_t recv_data(TCP_conn* conn, TCP_msg* msg)
{
    errval_t err;
    assert(conn && msg);
    assert(conn->state == ESTABLISHED);

    switch (msg->flags) {
    case TCP_FLAG_PSH_ACK:  // Same as ACK;
    case TCP_FLAG_ACK: {

        assert(msg->seqno == conn->recvno);
        assert(msg->ackno == conn->sendno);
        conn->recvno += msg->buf.valid_size;

        TCP_msg ret_msg = {
            .send = {
                .dst_ip   = conn->src_ip,
                .dst_port = conn->src_port,
            },
            .flags = TCP_FLAG_ACK,
            .seqno = conn->sendno,
            .ackno = conn->recvno,
            .window = 65535,
            .urg_ptr = 0,
            .buf   = NULL_BUFFER,
        };

        err = server_send(conn->server, &ret_msg);
        DEBUG_FAIL_RETURN(err, "Can't marshal this message by server !");

        //TODO: what if size == 0 ?
        if (msg->buf.valid_size != 0) {
            err = server_callback(conn, msg->buf);
            DEBUG_FAIL_RETURN(err, "Can't send back this message by call back !");
        }
        break;
    }
    case TCP_FLAG_FIN_ACK: ///TODO: We should deal with the data
    case TCP_FLAG_FIN: {
        assert(msg->seqno == conn->recvno);
        assert(msg->ackno == conn->sendno);

        conn->state = CLOSE_WAIT;
        conn->recvno += 1;
        //TODO: just use FIN_ACK

        TCP_msg ret_msg = {
            .send = {
                .dst_ip   = conn->src_ip,
                .dst_port = conn->src_port,
            },
            .flags = TCP_FLAG_ACK,
            .seqno = conn->sendno,
            .ackno = conn->recvno,
            .window = 65535,
            .urg_ptr = 0,
            .buf   = NULL_BUFFER,
        };

        err = server_send(conn->server, &ret_msg);
        DEBUG_FAIL_RETURN(err, "Can't marshal the ACK to FIN by server !");

        conn->state = LAST_ACK;

        TCP_msg fin_msg = {
            .send = {
                .dst_ip   = conn->src_ip,
                .dst_port = conn->src_port,
            },
            .flags = TCP_FLAG_FIN,
            .seqno = conn->sendno,
            .ackno = conn->recvno,
            .window = 65535,
            .urg_ptr = 0,
            .buf   = NULL_BUFFER,
        };

        err = server_send(conn->server, &fin_msg);
        DEBUG_FAIL_RETURN(err, "Can't marshal the ACK to FIN by server !");
        break;
    }
    default: return NET_ERR_TCP_BAD_STATE;
    } 
    return SYS_ERR_OK;
}

static void conn_delete(TCP_conn* conn)
{
    assert(conn);
    if (!(conn->state == TIME_WAIT || conn->state == LAST_ACK)) {
        LOG_WARN("Abrupt closing a connection");
        dump_tcp_conn(conn);
    }

    // uint64_t key = TCP_CONN_KEY(conn->src_ip, conn->src_port);
    // (void) key;
    // collections_hash_delete(conn->server->connections, key);
}

static errval_t conn_normal_close(TCP_conn* conn, TCP_msg* msg)
{   
    errval_t err = SYS_ERR_OK;
    assert(conn);

    switch(conn->state) {
    case LAST_ACK:
        switch (msg->flags) {
        case TCP_FLAG_ACK:
            assert(msg->seqno == conn->recvno);
            assert(msg->ackno == conn->sendno);

            conn->state = CLOSED;

            conn_delete(conn);
            LOG_ERR("Server closed");

            break;
        default: err = NET_ERR_TCP_BAD_STATE;
        }
        break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
    case CLOSING:
        err = conn_normal_close(conn, msg);
        break;
    default: USER_PANIC("Shouldn't be here");
    }

    return err;
}

static void conn_abrupt_close(TCP_conn* conn)
{
    assert(conn);
    TCP_msg rst_msg = {
        .send    = {
            .dst_ip   = conn->src_ip,
            .dst_port = conn->src_port,
        },
        .flags   = TCP_FLAG_RST,
        .seqno   = conn->sendno,
        .ackno   = conn->recvno,
        .window  = 65535,
        .urg_ptr = 0,
        .buf     = NULL_BUFFER,
    };
    errval_t err = server_send(conn->server, &rst_msg);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "Can't marshal the RST message by server !");
    }
}

errval_t conn_handle_msg(TCP_conn* conn, TCP_msg* msg) 
{
    errval_t err = SYS_ERR_OK;
    assert(conn && msg);

    switch (msg->flags) {
    case TCP_FLAG_RST_ACK:
    case TCP_FLAG_RST:
        dump_tcp_msg(msg);
        dump_tcp_conn(conn);
        assert(msg->seqno == conn->recvno);
        assert(msg->ackno == 0);
        conn_delete(conn);
        LOG_ERR("An abrupt stop");
        return SYS_ERR_OK;
    default:    // Continue processing
    }

    switch (conn->state) {
    case CLOSED:        // It means we don't accept connection
        err = NET_ERR_TCP_BAD_STATE;
        break;
    case LISTEN:        // Server Establish Connection
    case SYN_RECVD:
        err = server_handshake_open(conn, msg);
        break;
    case SYN_SENT:      // Client Establish Connection
        err = NET_ERR_TCP_BAD_STATE;
        break;
    case ESTABLISHED:   // Both Exchange Data
        err = recv_data(conn, msg);
        break;
    case LAST_ACK:      // Close Connection
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
    case CLOSING:
        err = conn_normal_close(conn, msg);
        break;
    default:
        LOG_ERR("Unknown Connection state: %d", conn->state);
        break;
    }

    if (err_no(err) == NET_ERR_TCP_BAD_STATE) {
        LOG_ERR("We have a bad state, abrupt stop");
        dump_tcp_msg(msg);
        dump_tcp_conn(conn);
        conn_abrupt_close(conn);
        conn_delete(conn);
    } else 
        DEBUG_FAIL_RETURN(err, "Some error when handling this message");
    
    free_message(msg);
    return SYS_ERR_OK;
}

////////////////////////////////////////////////////////////////////////////
/// Dump functions
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

// Function to dump the contents of a TCP_msg struct
void dump_tcp_msg(const TCP_msg *msg) {
    char src_ip_str[IPv6_ADDRESTRLEN];
    format_ip_addr(msg->recv.src_ip, src_ip_str, IPv6_ADDRESTRLEN);

    printf("TCP Message:\n");
    printf("   Source IP: %s\n", src_ip_str);
    printf("   Source Port: %u\n", msg->recv.src_port);
    printf("   Flags: %s\n", flags_to_string(msg->flags));
    printf("   Sequence Number: %u\n", msg->seqno);
    printf("   Acknowledgment Number: %u\n", msg->ackno);
    printf("   Data Size: %u bytes\n", msg->buf.valid_size);
}