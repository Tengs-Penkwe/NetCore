#include "ip_gather.h"
#include <event/timer.h>
#include <event/threadpool.h>
#include <netutil/dump.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
KAVLL_INIT(Mseg, Mseg, head, seg_cmp)
#pragma GCC diagnostic pop

void drop_message(void* message) {
    IP_recv* msg = message; assert(msg);
    IP* ip = msg->ip; assert(ip);

    ip_msg_key_t msg_key = MSG_KEY(msg->src_ip, msg->id);
    khint64_t key = kh_get(ip_recv, ip->recv_messages, msg_key);

    if (key == kh_end(ip->recv_messages) && msg->alloc_size != 0)
        USER_PANIC("The message doesn't exist in hash table before we delete it!");

    if (msg->alloc_size != 0) { // Means there was assembling process
        // Delete the message from the hash table
        pthread_mutex_lock(&ip->mutex);
        kh_del(ip_recv, ip->recv_messages, msg_key);
        pthread_mutex_unlock(&ip->mutex);
        IP_NOTE("Deleted a message from hash table");

        // Free all segments
        assert(msg->seg);
        kavll_free(Mseg, head, msg->seg, free);
        msg->seg = NULL;

        USER_PANIC("TODO: free all the data");

        // Note: it doesn't matter if we use recvd_size or sent.size since we used the union
        assert(msg->whole_size && msg->recvd_size);
        // Free the allocated data:
        free(msg->data);
        // if message come here directly, data should be free'd in lower module

        // Destroy mutex
        pthread_mutex_destroy(&msg->mutex);
    }

    assert(msg->seg == NULL);
    // Free the message itself and set it to NULL
    free(message);
    message = NULL;
}

/**
 * @brief Checks the status of an IP message, drops it if TTL expired, or processes it if complete.
 *        This functions is called in a periodic event
 * 
 * @param message Pointer to the IP_recv structure to be checked.
 */
void check_recvd_message(void* message) {
    IP_VERBOSE("Checking a message");
    errval_t err;
    IP_recv* msg = message; assert(msg);

    /// Also modified in ip_assemble(), be careful of global states
    msg->times_to_live *= 1.5;
    if (msg->times_to_live >= IP_GIVEUP_RECV_US)
    {
        drop_message(msg);
    }
    else
    {
        if (msg->recvd_size == msg->whole_size) { // We can process the package now
            // We don't need to care about duplicate segment here, they are deal in ip_assemble
            IP_DEBUG("We spliced an IP message of size %d, ttl: %d, now let's process it", msg->whole_size, msg->times_to_live);

            err = ip_handle(msg);
            if (err_is_fail(err)) { DEBUG_ERR(err, "We meet an error when handling an IP message"); }
            drop_message(msg);
        }
        else
        {
            submit_delayed_task(msg->times_to_live, MK_TASK(check_recvd_message, (void *)msg));
            IP_VERBOSE("Done Checking a message, ttl: %d, whole size: %p", msg->times_to_live, msg->whole_size);
        }
    }
}

/**
 * @brief Assemble an IP packet to message
 *
 * @param[in]   bind        IP_binding struct stores all the states
 * @param[in]   id          Identification number of the message.
 * @param[in]   msg_len     Length of the whole message
 * @param[in]   data        The start of the bufffer to copy from
 * @param[in]   size        Length of this packet
 *
 * @return Returns error code indicating success or failure.
 */
errval_t ip_assemble(
    IP* ip, ip_addr_t src_ip, uint8_t proto, uint16_t id, uint8_t* addr, size_t size, uint32_t offset, bool more_frag, bool no_frag
) {
    // errval_t err;
    assert(ip && addr);
    IP_DEBUG("Assembling a message, ID: %d, addr: %p, size: %d, offset: %d, no_frag: %d, more_frag: %d", id, addr, size, offset, no_frag, more_frag);

    ip_msg_key_t msg_key = MSG_KEY(src_ip, id);
    IP_recv* msg = NULL;

    ///TODO: add lock ?
    khint64_t key = kh_get(ip_recv, ip->recv_messages, msg_key);
    // Try to find if it already exists
    if (key == kh_end(ip->recv_messages)) {  // This message doesn't exist yet
        msg = malloc(sizeof(IP_recv)); assert(msg);
        *msg = (IP_recv) {
            .ip         = ip,
            .proto      = proto,
            .id         = id,
            .src_ip = src_ip,
            .mutex      = { { 0 } },
            .whole_size = SIZE_DONT_KNOW,     // We don't know the size util the last packet arrives
            .alloc_size = 0,
            .seg        = NULL,
            .data       = NULL,
            .recvd_size = size,
            .times_to_live = IP_RETRY_RECV_US,
        };

        ///TODO: What if 2 threads received duplicate segmentation at the same time ?
        if (no_frag == true || (more_frag == false && offset == 0)) { // Which means this isn't a segmented packet
            assert(offset == 0 && more_frag == false);

            msg->data = addr;
            assert(msg->alloc_size == 0);
            assert(msg->whole_size == SIZE_DONT_KNOW);
            msg->whole_size = size;
            check_recvd_message((void*) msg);

            return SYS_ERR_OK;
        }

        // Multiple threads may handle different segmentation of IP at same time, need to deal with it 
        if (pthread_mutex_init(&msg->mutex, NULL) != 0) {
            IP_ERR("Can't initialize the mutex for IP message");
            return SYS_ERR_INIT_FAIL;
        }

        // Root of AVL tree
        msg->seg = malloc(sizeof(Mseg));  assert(msg->seg);
        msg->seg->offset = offset;

        ///TODO: Should I lock it before kh_get, if thread A is changing the hash table while thread
        // B is reading it, will it cause problem ? 
        pthread_mutex_lock(&ip->mutex);
        int ret;
        key = kh_put(ip_recv, ip->recv_messages, msg_key, &ret); 
        switch (ret) {
        case -1:    // The operation failed
            pthread_mutex_unlock(&ip->mutex);
            USER_PANIC("Can't add a new message with seqno: %d to hash table", id);
        case 1:     // the bucket is empty 
        case 2:     // the element in the bucket has been deleted 
        case 0: 
            break;
        default:    USER_PANIC("Can't be this case: %d", ret);
        }
        // Set the value of key
        kh_value(ip->recv_messages, key) = msg;
        pthread_mutex_unlock(&ip->mutex);
        
    } else {
        // TODO: should I accquire the lock ?
        msg = kh_val(ip->recv_messages, key); assert(msg);
        assert(msg->proto == proto);
        ///ALARM: global state, also modified in check_recvd_message
        msg->times_to_live /= 1.5;  // We got 1 more packet, wait less time

        Mseg* seg = malloc(sizeof(Mseg)); assert(seg);
        seg->offset = offset;
        if (seg != Mseg_insert(&msg->seg, seg)) { // Already exists !
            IP_ERR("We have duplicate IP message segmentation with offset: %d", seg->offset);
            // TODO: this assumes header size is 20
            dump_ipv4_header((const struct ip_hdr*) (addr - sizeof(struct ip_hdr)));
            free(seg);
            return NET_ERR_IPv4_DUPLITCATE_SEG;
        }
    }

    uint32_t needed_size = offset + size;
    // If this the laset packet, we now know the final size
    if (more_frag == false) {
        assert(no_frag == false);
        assert(msg->whole_size == SIZE_DONT_KNOW);
        msg->whole_size = offset + size;
    }
    // If the current size is smaller, we need to re-allocate space
    if (msg->alloc_size < needed_size) {
        msg->alloc_size = needed_size;
        if (msg->data == NULL) {
            msg->data = malloc(msg->alloc_size); assert(msg->data);
        } else {
            msg->data = realloc(msg->data, needed_size); assert(msg->data);
        }
    }
    // Copy the message to the buffer
    memcpy(msg->data + offset, (void*)addr, size);
    msg->recvd_size += size; 
    IP_VERBOSE("Done Assembling a packet of a message");

    // If the message is complete, we trigger the check message,
    if (msg->recvd_size == msg->whole_size) {
        IP_ERR("Completed");
        check_recvd_message((void*) msg);
    } else {
        IP_WARN("Later");
        submit_delayed_task(msg->times_to_live, MK_TASK(check_recvd_message, (void*)msg));
    }
    
    return SYS_ERR_OK;
}

/**
 * @brief Unmarshals a complete IP message and processes it based on its protocol type.
 * 
 * @param msg Pointer to the IP message to be unmarshalled.
 * 
 * @return Returns error code indicating success or failure.
 */
errval_t ip_handle(IP_recv* msg) {
    errval_t err;
    IP_VERBOSE("An IP Message has been assemble, now let's process it");

    switch (msg->proto) {
    case IP_PROTO_ICMP:
        IP_VERBOSE("Received a ICMP packet");
        err = icmp_unmarshal(msg->ip->icmp, msg->src_ip, msg->data, msg->recvd_size);
        RETURN_ERR_PRINT(err, "Error when unmarshalling an ICMP message");
        break;
    case IP_PROTO_UDP:
        IP_VERBOSE("Received a UDP packet");
        // err = udp_unmarshal(msg->ip->udp, msg->src_ip, msg->, msg->recvd_size);
        // RETURN_ERR_PRINT(err, "Error when unmarshalling an UDP message");
        break;
    case IP_PROTO_IGMP:
    case IP_PROTO_UDPLITE:
        LOG_ERR("Unsupported Protocal");
        return SYS_ERR_NOT_IMPLEMENTED;
    case IP_PROTO_TCP:
        IP_VERBOSE("Received a TCP packet");
        // err = tcp_unmarshal(msg->ip->tcp, msg->src_ip, msg->, msg->recvd_size);
        // RETURN_ERR_PRINT(err, "Error when unmarshalling an TCP message");
        break;
    default:
        LOG_ERR("Unknown packet type for IPv4: %p", msg->proto);
        return NET_ERR_IPv4_WRONG_PROTOCOL;
    }

    IP_VERBOSE("Done unmarshalling a IPv4 message");
    return SYS_ERR_OK;
}
