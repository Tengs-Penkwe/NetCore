#ifndef __IPC_SHARE_MEM_H__
#define __IPC_SHARE_MEM_H__

#include <common.h>
#include <stdatomic.h>
#include "type.h"

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

#endif // __IPC_SHARE_MEM_H__
