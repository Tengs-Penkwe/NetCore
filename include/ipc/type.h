#ifndef __IPC_TYPE_H__
#define __IPC_TYPE_H__

#include <common.h>
#include <netutil/ip.h>     //ip_addr_t
#include <netutil/udp.h>    //udp_port_t
#include <log.h>
#include <stdatomic.h>

/*
 * ------------------------------------------------------------------------------------------------
 * RPC: Definitions of message
 * ------------------------------------------------------------------------------------------------
 */

/// defines the transport backend of the RPC channel
typedef enum {
    RPC_SHRM = 1,
    RPC_PIPE = 2,
} rpc_type_t;

typedef enum rpc_cmd {
    CMD_CONFIRM_BINDING,
    CMD_SEND_NUMBER,
    CMD_SEND_STRING,
    CMD_SEND_CHAR,
    CMD_REGISTER_SERVER,
    RQR_GET_CHAR, 
    RQR_PROC_TERMINATED,
    RQR_FS_CREATE_SOCKET,
    CMD_NET_SEND_PACKET,
    CMD_NET_BACK_PACKET,
    CMD_NET_BIND_PORT,
    CMD_NET_TCP_LISTEN,
    CMD_NET_ACCEPT_BIND,
    CMD_NET_ARP_DUMP,
    BCK_ERROR,
} rpc_cmd_t;

typedef enum validity {
    VALID   = 0b01,
    INVALID = 0b10,
} validity_t;

typedef struct rpc_message {
    struct {
        uint64_t valid  : 2;     ///< if this message is valid
        uint64_t cmd    : 6;    ///< type of message
        uint64_t length : 32;   ///< length of data, if > 56, use frame
    } meta;               // 8 bytes
    uint8_t payload[56];  // 56 bytes
}rpc_message_t;

#define RPC_PAYLOAD_FULL_SIZE sizeof(((struct ipc_message *)0)->payload)

static_assert(sizeof(rpc_message_t) == 64);

/// type of the receive handler function.
typedef void (*rpc_recv_handler_fn)(void *ac);

/*
 * ------------------------------------------------------------------------------------------------
 * RPC: Shared Memory
 * ------------------------------------------------------------------------------------------------
 */
#define URPC_SHRM_SIZE      8192

/// urpc frame holds two rings, sending one (tx) and receiving one (rx)
#define RPC_SHRM_RING_ENTRIES  ((URPC_SHRM_SIZE / 2 - 32) / sizeof(rpc_message_t)) 
// We need space for lock, head, tail

typedef struct share_mem {
    size_t    base;
    size_t    size;
} ShareMem;

typedef struct shrm_msg_ring {
    atomic_flag        lock;
    uint8_t            head;
    uint8_t            tail;
    struct rpc_message ring[RPC_SHRM_RING_ENTRIES];
} shrm_msg_ring_t;

struct rpc_shrm {
    struct share_mem      shrm; ///< Created when booting core, it only holds 2 rings
    struct shrm_msg_ring *tx;   ///< Point into urpc_frame, place depends on if the process is initializer
    struct shrm_msg_ring *rx;   ///< same as tx
};

/*
 * ------------------------------------------------------------------------------------------------
 * RPC: Message Type
 * ------------------------------------------------------------------------------------------------
 */

typedef struct message_server {
    union {
        struct {
            int domain;
            int type;
            int protocal;
        }  __attribute__((packed)) fs_create_socket;
        struct {
            int fd;
        }  __attribute__((packed)) send_fs_create_socket;
        struct {
            int        fd;
            ip_addr_t  dst_ip;
            udp_port_t dst_port;
            size_t     size;
        }  __attribute__((packed)) net_send_packet;
        struct {
            int        fd;
            ip_addr_t  ip_to_recv;
            udp_port_t port_to_bind;
        }  __attribute__((packed)) net_bind_port;
        struct {
            int        fd;
            ip_addr_t  src_ip;
            udp_port_t src_port;
            size_t     size;
        }  __attribute__((packed)) net_back_packet;
        struct {
            int        fd;
            int        max_conn;
        }  __attribute__((packed)) net_tcp_listen;
    };
} __attribute__((packed)) msg_server_t ; 

typedef struct message_client {
    union {
        struct {
            int* fd;
        } __attribute__((packed)) fs_create_socket;
        struct {
            struct capref*  remotep;
        }__attribute__((packed)) lmp_forward_bind;
        struct {
            size_t len;
            char*  name;
        }  __attribute__((packed)) proc_get_name;
    };
} __attribute__((packed)) msg_client_t;

/**
 * @brief ID of a rpc channel
 *
 * Note: core and domain is the primary key, name is current not well-used
 */
typedef struct rpc_id {
    const char* name;
} rpc_id_t;

/**
 * @brief represents an RPC binding
 *
 * Note: the RPC binding should work over LMP (M4) or UMP (M6)
 */
typedef struct rpc {
    /// Global
    rpc_type_t type;
    rpc_id_t   id;
    union {
        struct rpc_shrm shrm;
    };
} rpc_t;

static inline void message_info(rpc_message_t* msg) {
    IPC_ERR("RPC Message:\n "
        "\t Command:0x%x \t Valid: 0x%x \t Length: %d\n",
        msg->meta.cmd, msg->meta.valid, msg->meta.length);
}

static inline void rpc_info(rpc_t* rpc) {
    char* type;
    if (rpc->type == RPC_SHRM) {
        type = "Shared Memory"; 
    } else if (rpc->type == RPC_PIPE) {
        type = "Pipe";
    } else {
        type = "UNKNOWN";
    }
    IPC_ERR("AOS_RPC: %s\n "
        "\t Name:%s\n", type, rpc->id.name);
}

#endif  // __IPC_TYPE_H__