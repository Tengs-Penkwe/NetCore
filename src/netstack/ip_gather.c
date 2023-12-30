#include "ip_gather.h"
#include <event/timer.h>
#include <event/threadpool.h>
#include <netutil/dump.h>
#include <event/states.h>
#include <sys/syscall.h>   //syscall    
#include <event/event.h>   //event_ip_handle

static void* gather_thread(void* state);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
KAVLL_INIT(Mseg, Mseg, head, seg_cmp)
#pragma GCC diagnostic pop

/// @brief      Initialize the gatherer thread
/// @param gather       Pointer to the gatherer thread
/// @param queue_size   Size of the message queue
/// @param id           ID of the gatherer thread
/// @return     Error code
errval_t gather_init(
    IP_gatherer* gather, size_t queue_size, size_t id
) {
    errval_t err = SYS_ERR_OK;
    
    // 1. Initialize the message queue
    BQelem * queue_elements = calloc(queue_size, sizeof(BQelem));
    err = bdqueue_init(&gather->event_que, queue_elements, queue_size);
    if (err_is_fail(err)) {
        IP_FATAL("Can't Initialize the queues for TCP messages, TODO: free the memory");
        return err_push(err, SYS_ERR_INIT_FAIL);
    }
    
    // 1.1 initialize the hash table
    gather->recv_messages = kh_init(ip_msg);
        
    // 2. Initialize the semaphore for senders
    if (sem_init(&gather->event_come, 0, 0) != 0) {
        IP_FATAL("Can't Initialize the semaphores for IP segmented messages, TODO: free the memory");
        return SYS_ERR_INIT_FAIL;
    }

    // 3.1 Local state for the gather thread
    char* name = calloc(32, sizeof(char));
    sprintf(name, "IP Gatherer %d", id);

    LocalState* local = calloc(1, sizeof(LocalState)); 
    *local = (LocalState) {
        .my_name  = name,
        .my_pid   = (pid_t)-1,      // Don't know yet
        .log_file = (g_states.log_file == NULL) ? stdout : g_states.log_file,
        .my_state = gather,             // Provide message queue and semaphore
    };

    // 3.2 Create the gather thread
    if (pthread_create(&gather->self, NULL, gather_thread, (void*)local) != 0) {
        TCP_FATAL("Can't create worker thread, TODO: free the memory");
        return EVENT_ERR_THREAD_CREATE;
    }
    
    return err;
}

void gather_destroy(
    IP_gatherer* gather
) {
    assert(gather);

    bdqueue_destroy(&gather->event_que);
    sem_destroy(&gather->event_come);
    pthread_cancel(gather->self);
    
    IP_ERR("TODO: free the memory");
    // kavll_free(Mseg, head, gather->seg, free);
    free(gather);
}

/// @brief     The gatherer thread
/// We need to make sure that the segmented messages are processed in single-thread manner, 
/// to handle out-of-order, duplicate, and missing segments in multi-thread is too complicated,
/// and requires significant resource, which is not worth it.
static void* gather_thread(void* state) {
    LocalState* local = state; assert(local);
    local->my_pid = syscall(SYS_gettid);
    set_local_state(local);
    IP_NOTE("%s started with pid %d", local->my_name, local->my_pid);

    CORES_SYNC_BARRIER;

    IP_gatherer* gather = local->my_state; assert(gather);
    
    Task* task = NULL;
    while (true)
    {
        if (debdqueue(&gather->event_que, NULL, (void**)&task) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&gather->event_come);
        } else {
            assert(task);
            (task->process)(task->arg);
            free(task);
            task = NULL;
        }
    }
}

/// @brief delete the message from the hash table, and free the memory
/// 1. Assumption: single thread
/// 2. DO free recv itself
void drop_recvd_message(void* message) {
    IP_recv* recv = message; assert(recv);
    IP_gatherer* gather = recv->gatherer; assert(gather);

    ip_msg_key_t msg_key = IP_MSG_KEY(recv->src_ip, recv->id);
    khint64_t key = kh_get(ip_msg, gather->recv_messages, msg_key);

    if (key == kh_end(gather->recv_messages))
        USER_PANIC("The message doesn't exist in hash table before we delete it!");

    // Delete the message from the hash table
    kh_del(ip_msg, gather->recv_messages, msg_key);

    if (recv->whole_size != recv->recvd_size)
        IP_WARN("We drop a message that is not complete, size: %d, received: %d", recv->whole_size, recv->recvd_size);

    // can't use kval_free, because we need to free the buffer
    // kval_free(Mseg, head, msg->whole.seg, free);

    Mseg_itr_t seg_itr = { 0 };
    Mseg_itr_first(recv->seg, &seg_itr); // Initialize the iterator

    uint16_t offset = 0;
    do {
        const Mseg *node = kavll_at(&seg_itr);
        assert(offset == node->offset);   
        offset += node->buf.valid_size;

        free_buffer(node->buf);     
        free((void*)node);
    } while (Mseg_itr_next(&seg_itr));

    free(message);
}

/// 1. Assumption: single thread
/// 2. DO NOT free recv itself
/// @brief Assemble the segments into a single buffer, free the memory of the segments
static Buffer segment_assemble(IP_recv* recv) {
    assert(recv && recv->seg);

    // 1. We are assmebling a complete message, so th size must match   
    uint16_t whole_size = recv->whole_size;
    assert(recv->recvd_size == whole_size);

    // 2. Reserve some space if the receiver want to re-use the buffer
    uint8_t* all_data = malloc(whole_size + DEVICE_HEADER_RESERVE); assert(all_data);
    all_data += DEVICE_HEADER_RESERVE;
    
    // 3. Create the buffer
    Buffer ret_buf =  {
        .data       = all_data,
        .from_hdr   = DEVICE_HEADER_RESERVE, 
        .valid_size = whole_size,
        .whole_size = whole_size + DEVICE_HEADER_RESERVE,
        .from_pool  = false,
        .mempool    = NULL,
    };

    // 4. Traverse the AVL tree, and copy the data to the buffer
    Mseg_itr_t seg_itr = { 0 };
    Mseg_itr_first(recv->seg, &seg_itr); // Initialize the iterator

    uint16_t offset = 0;
    do {
        const Mseg *node = kavll_at(&seg_itr);
        assert(offset == node->offset);   

        memcpy(all_data + offset, node->buf.data, node->buf.valid_size);
        offset += node->buf.valid_size;

        free_buffer(node->buf);
        free((void*)node);
    } while (Mseg_itr_next(&seg_itr));

    // free(recv); // We don't free it here, because it's not malloc'd here
    
    return ret_buf;
}

/** Assumption: single thread
 *
 * @brief Checks the status of an IP message, drops it if TTL expired, or processes it if complete.
 *        This functions is called in a periodic event
 * @param message Pointer to the IP_segment structure to be checked.
 */
void check_recvd_message(void* message) {
    IP_VERBOSE("Checking a message");
    errval_t err = SYS_ERR_OK;
    IP_recv* recv = message; assert(recv);
    IP_gatherer* gather = recv->gatherer; assert(gather);

    /// Also modified in ip_assemble(), be careful of global states
    recv->times_to_live *= 1.5;
    if (recv->times_to_live >= IP_GIVEUP_RECV_US)
    {
        drop_recvd_message(recv);
    }
    else
    {
        assert(recv->recvd_size <= recv->whole_size);
        if (recv->recvd_size == recv->whole_size) { // We can process the package now
            cancel_timer_task(recv->timer);

            // We don't need to care about duplicate segment here, they are deal in ip_assemble
            IP_DEBUG("We spliced an IP message of size %d, ttl: %d, now let's process it", recv->whole_size, recv->times_to_live / 1000);
            
            Buffer buf = segment_assemble(recv);
            IP_handle* handle = malloc(sizeof(IP_handle)); assert(handle);
            *handle = (IP_handle) {
                .ip     = gather->ip,
                .proto  = recv->proto,
                .src_ip = recv->src_ip,
                .buf    = buf,
            };
            free(recv);
            err = submit_task(MK_NORM_TASK(event_ip_handle, (void*)handle));
            if (err_is_fail(err)) {
                DEBUG_ERR(err, "We assembled an IP message, but can't submit it as an event, will drop it");
                free_buffer(buf);
                free(handle);
            }
        }
        else
        {
            IP_VERBOSE("Done Checking a message, ttl: %d ms, whole size: %d, received %d", recv->times_to_live / 1000, recv->whole_size, recv->recvd_size);
            // Here we submit a delayed task to check the message again to ourself, this is to ensure that the message is processed in single thread
            Task task_for_myself = MK_TASK(&gather->event_que, &gather->event_come, check_recvd_message, (void*)recv);
            recv->timer = submit_delayed_task(MK_DELAY_TASK(recv->times_to_live, drop_recvd_message, task_for_myself));
        }
    }
}

/// This is also a event, be careful of error codes
errval_t ip_gather(IP_segment* segment)
{
    assert(segment);
    IP_gatherer* gather = segment->gatherer; assert(gather);
    // dump_buffer(segment->buf);

    IP_recv *recv = NULL;

    uint16_t recvd_size = segment->buf.valid_size;
    uint16_t offset     = segment->offset;
    bool     no_frag    = segment->no_frag;
    bool     more_frag  = segment->more_frag;
    Buffer   buf        = segment->buf;

    ip_msg_key_t msg_key = IP_MSG_KEY(segment->src_ip, segment->id);
    khint64_t key = kh_get(ip_msg, gather->recv_messages, msg_key);
    // Try to find if it already exists
    if (key == kh_end(gather->recv_messages)) {  // This message doesn't exist in the hash table of the gatherer

        recv = malloc(sizeof(IP_recv)); assert(recv);
        *recv = (IP_recv) {
            .gatherer      = segment->gatherer,
            .src_ip        = segment->src_ip,
            .proto         = segment->proto,
            .id            = segment->id,
            .whole_size    = SIZE_DONT_KNOW,        // We don't know the size util the last packet arrives
            .recvd_size    = recvd_size,
            .seg           = malloc(sizeof(Mseg)),  // Root of AVL tree
            .timer         = 0,
            .times_to_live = IP_GIVEUP_RECV_US,
        };
        assert(recv->seg);

        recv->seg->offset = offset;
        recv->seg->buf    = buf;

        int ret;
        key = kh_put(ip_msg, gather->recv_messages, msg_key, &ret); 
        switch (ret) {
        case -1:    // The operation failed
            USER_PANIC("Can't add a new message with seqno: %d to hash table", recv->id);
            free(recv->seg);
            free(recv);
        case 1:     // the bucket is empty 
        case 2:     // the element in the bucket has been deleted 
            break;
        case 0:     // Already in the hash table
        default: 
            USER_PANIC("Can't be this case: %d", ret);
            free(recv->seg);
            free(recv);
        }
        // Set the value of key
        kh_value(gather->recv_messages, key) = recv;
        
    } else {

        recv = kh_val(gather->recv_messages, key); assert(recv);
        assert(segment->proto == recv->proto);

        ///ALARM: global state, also modified in check_recvd_message()
        recv->times_to_live /= 1.5;  // We got 1 more packet, wait less time

        Mseg *seg = malloc(sizeof(Mseg)); assert(seg);
        seg->offset = offset;
        seg->buf    = buf;

        if (seg != Mseg_insert(&recv->seg, seg)) { // Already exists !
            IP_ERR("We have duplicate IP message segmentation with offset: %d", seg->offset);
            // Will be freed in upper module (event caller)
            // free_buffer(buf);
            free(seg);
            return NET_ERR_IPv4_DUPLITCATE_SEG;
        }
        // The operation above assure there is no duplicate segment
        recv->recvd_size += recvd_size;
    }

    // If this the last packet, we now know the final size
    if (more_frag == false) {
        assert(no_frag == false);
        assert(recv->whole_size == SIZE_DONT_KNOW);
        recv->whole_size = offset + recvd_size;
    }

    assert(recv->recvd_size <= recv->whole_size);
    // If the message is complete, we trigger the check message,
    if (recv->recvd_size == recv->whole_size)
    {
        IP_DEBUG("We have received a complete IP message of size %d, now let's process it", recv->whole_size);
        check_recvd_message((void*) recv);
        return NET_THROW_IPv4_ASSEMBLE;
    }
    else
    {
        IP_DEBUG("We have received %d bytes of a message of size %d, now let's wait for %d ms", recv->recvd_size, recv->whole_size, recv->times_to_live / 1000);
        Task task_for_myself = MK_TASK(&gather->event_que, &gather->event_come, check_recvd_message, (void*)recv);
        recv->timer = submit_delayed_task(MK_DELAY_TASK(recv->times_to_live, drop_recvd_message, task_for_myself));
        return NET_THROW_SUBMIT_EVENT;
    }
}

/**
 * @brief Unmarshals a complete IP message and processes it based on its protocol type.
 * 
 * @param msg Pointer to the IP message to be unmarshalled.
 * 
 * @return Returns error code indicating success or failure.
 */
errval_t ip_handle(IP* ip, uint8_t proto, ip_addr_t src_ip, Buffer buf) {
    errval_t err = SYS_ERR_OK;
    IP_VERBOSE("An IP Message has been assemble, now let's process it");

    switch (proto) {
    case IP_PROTO_ICMP:
        IP_VERBOSE("Received a ICMP packet");
        err = icmp_unmarshal(ip->icmp, src_ip, buf);
        DEBUG_FAIL_RETURN(err, "Error when unmarshalling an ICMP message");
        return err;
    case IP_PROTO_UDP:
        IP_VERBOSE("Received a UDP packet");
        err = udp_unmarshal(ip->udp, src_ip, buf);
        DEBUG_FAIL_RETURN(err, "Error when unmarshalling an UDP message");
        return err;
    case IP_PROTO_IGMP:
    case IP_PROTO_UDPLITE:
        LOG_ERR("Unsupported Protocal");
        return SYS_ERR_NOT_IMPLEMENTED;
    case IP_PROTO_TCP:
        IP_VERBOSE("Received a TCP packet");
        // err = tcp_unmarshal(msg->ip->tcp, msg->src_ip, msg->, msg->whole.recvd);
        // DEBUG_FAIL_RETURN(err, "Error when unmarshalling an TCP message");
        return SYS_ERR_NOT_IMPLEMENTED;
    default:
        LOG_ERR("Unknown packet type for IPv4: %p", proto);
        return NET_ERR_IPv4_WRONG_PROTOCOL;
    }
}
