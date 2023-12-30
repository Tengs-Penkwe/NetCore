#include "ip_assemble.h"
#include <event/timer.h>
#include <event/threadpool.h>
#include <netutil/dump.h>
#include <event/states.h>
#include <sys/syscall.h>   //syscall    
#include <event/event.h>   //event_ip_handle

static void* assemble_thread(void* state);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
KAVLL_INIT(Mseg, Mseg, head, seg_cmp)
#pragma GCC diagnostic pop

/// @brief      Initialize the assembler thread
/// @param assemble       Pointer to the assembler thread
/// @param queue_size   Size of the message queue
/// @param id           ID of the assembler thread
/// @return     Error code
errval_t assemble_init(
    IP_assembler* assemble, size_t queue_size, size_t id
) {
    errval_t err = SYS_ERR_OK;
    
    // 1. Initialize the message queue
    BQelem * queue_elements = calloc(queue_size, sizeof(BQelem));
    err = bdqueue_init(&assemble->event_que, queue_elements, queue_size);
    if (err_is_fail(err)) {
        IP_FATAL("Can't Initialize the queues for TCP messages, TODO: free the memory");
        return err_push(err, SYS_ERR_INIT_FAIL);
    }
    
    // 1.1 initialize the hash table
    assemble->recv_messages = kh_init(ip_msg);
        
    // 2. Initialize the semaphore for senders
    if (sem_init(&assemble->event_come, 0, 0) != 0) {
        IP_FATAL("Can't Initialize the semaphores for IP segmented messages, TODO: free the memory");
        return SYS_ERR_INIT_FAIL;
    }

    // 3.1 Local state for the assemble thread
    char* name = calloc(32, sizeof(char));
    sprintf(name, "IP Gatherer %d", id);

    LocalState* local = calloc(1, sizeof(LocalState)); 
    *local = (LocalState) {
        .my_name  = name,
        .my_pid   = (pid_t)-1,      // Don't know yet
        .log_file = (g_states.log_file == NULL) ? stdout : g_states.log_file,
        .my_state = assemble,             // Provide message queue and semaphore
    };

    // 3.2 Create the assemble thread
    if (pthread_create(&assemble->self, NULL, assemble_thread, (void*)local) != 0) {
        TCP_FATAL("Can't create worker thread, TODO: free the memory");
        return EVENT_ERR_THREAD_CREATE;
    }
    
    return err;
}

void assemble_destroy(
    IP_assembler* assemble
) {
    assert(assemble);

    bdqueue_destroy(&assemble->event_que);
    sem_destroy(&assemble->event_come);
    pthread_cancel(assemble->self);
    
    IP_ERR("TODO: free the memory");
    // kavll_free(Mseg, head, assemble->seg, free);
    free(assemble);
}

/// @brief     The assembler thread
/// We need to make sure that the segmented messages are processed in single-thread manner, 
/// to handle out-of-order, duplicate, and missing segments in multi-thread is too complicated,
/// and requires significant resource, which is not worth it.
static void* assemble_thread(void* state) {
    LocalState* local = state; assert(local);
    local->my_pid = syscall(SYS_gettid);
    set_local_state(local);
    IP_NOTE("%s started with pid %d", local->my_name, local->my_pid);

    CORES_SYNC_BARRIER;

    IP_assembler* assemble = local->my_state; assert(assemble);
    
    Task* task = NULL;
    while (true)
    {
        if (debdqueue(&assemble->event_que, NULL, (void**)&task) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&assemble->event_come);
        } else {
            assert(task);
            (task->process)(task->arg);
            free(task);
            task = NULL;
        }
    }
}

static void delete_msg_from_hash_table(IP_assembler* assemble, IP_recv* recv) {
    assert(assemble && recv);
    ip_msg_key_t msg_key = IP_MSG_KEY(recv->src_ip, recv->id);
    khint64_t key = kh_get(ip_msg, assemble->recv_messages, msg_key);

    if (key == kh_end(assemble->recv_messages))
        USER_PANIC("The message doesn't exist in hash table before we delete it!");

    // Delete the message from the hash table
    kh_del(ip_msg, assemble->recv_messages, key);
}

/// @brief delete the message from the hash table, and free the memory
/// 1. Assumption: single thread
/// 2. DO free recv itself
void drop_recvd_message(void* message) {
    IP_recv* recv = message; assert(recv && recv->seg);
    IP_assembler* assemble = recv->assembler; assert(assemble);
    
    // 1. remove the message from the hash table
    delete_msg_from_hash_table(assemble, recv);

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
static Buffer segment_assemble_and_delete_from_hash(IP_recv* recv) {
    assert(recv && recv->seg);
    IP_assembler* assemble = recv->assembler; assert(assemble);

    // 1. remove the message from the hash table
    delete_msg_from_hash_table(assemble, recv);

    // 2. We are assmebling a complete message, so th size must match   
    uint16_t whole_size = recv->whole_size;
    assert(recv->recvd_size == whole_size);

    // 3. Reserve some space if the receiver want to re-use the buffer
    uint8_t* all_data = malloc(whole_size + DEVICE_HEADER_RESERVE); assert(all_data);
    all_data += DEVICE_HEADER_RESERVE;
    
    // 4. Create the buffer
    Buffer ret_buf = buffer_create(
        all_data,
        DEVICE_HEADER_RESERVE, 
        whole_size,
        whole_size + DEVICE_HEADER_RESERVE,
        false,
        NULL
    );

    // 5. Traverse the AVL tree, and copy the data to the buffer
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
    IP_assembler* assemble = recv->assembler; assert(assemble);

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
            
            Buffer buf = segment_assemble_and_delete_from_hash(recv);
            IP_handle* handle = malloc(sizeof(IP_handle)); assert(handle);
            *handle = (IP_handle) {
                .ip     = assemble->ip,
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
            Task task_for_myself = MK_TASK(&assemble->event_que, &assemble->event_come, check_recvd_message, (void*)recv);
            recv->timer = submit_delayed_task(MK_DELAY_TASK(recv->times_to_live, drop_recvd_message, task_for_myself));
        }
    }
}

/// This is also a event, be careful of error codes
errval_t ip_assemble(IP_segment* segment)
{
    assert(segment);
    IP_assembler* assemble = segment->assembler; assert(assemble);
    // dump_buffer(segment->buf);

    IP_recv *recv = NULL;

    uint16_t recvd_size = segment->buf.valid_size;
    uint16_t offset     = segment->offset;
    bool     no_frag    = segment->no_frag;
    bool     more_frag  = segment->more_frag;
    Buffer   buf        = segment->buf;

    ip_msg_key_t msg_key = IP_MSG_KEY(segment->src_ip, segment->id);
    khint64_t key = kh_get(ip_msg, assemble->recv_messages, msg_key);
    // Try to find if it already exists
    if (key == kh_end(assemble->recv_messages)) {  // This message doesn't exist in the hash table of the assembler

        recv = malloc(sizeof(IP_recv)); assert(recv);
        *recv = (IP_recv) {
            .assembler     = segment->assembler,
            .src_ip        = segment->src_ip,
            .proto         = segment->proto,
            .id            = segment->id,
            .whole_size    = SIZE_DONT_KNOW,    // We don't know the size util the last packet arrives
            .recvd_size    = recvd_size,
            .seg           = 0,                 // Initialize the AVL tree
            .timer         = 0,
            .times_to_live = IP_RETRY_RECV_US,
        };

        // Insert the first segment to the AVL tree
        Mseg *seg = malloc(sizeof(Mseg)); assert(seg);
        seg->offset = offset;
        seg->buf    = buf;
        assert(seg == Mseg_insert(&recv->seg, seg));    // This is the first segment, so it must be inserted

        int ret;
        key = kh_put(ip_msg, assemble->recv_messages, msg_key, &ret); 
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
        kh_value(assemble->recv_messages, key) = recv;
        
        // Submit a delayed task to check the message 
        IP_DEBUG("We have received %d bytes of a message of size %d, now let's wait for %d ms", recv->recvd_size, recv->whole_size, recv->times_to_live / 1000);
        Task task_for_myself = MK_TASK(&assemble->event_que, &assemble->event_come, check_recvd_message, (void*)recv);
        recv->timer = submit_delayed_task(MK_DELAY_TASK(recv->times_to_live, drop_recvd_message, task_for_myself));
        
    } else {

        recv = kh_val(assemble->recv_messages, key); assert(recv);
        assert(segment->proto == recv->proto);

        ///ALARM: global state, also modified in check_recvd_message()
        recv->times_to_live = IP_RETRY_RECV_US; // Reset the TTL

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
        // cancel_timer_task(recv->timer);
        check_recvd_message((void*) recv);
    }
    return NET_THROW_IPv4_SEG;
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
