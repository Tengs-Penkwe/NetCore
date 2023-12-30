#include "ip_gather.h"
#include <event/timer.h>
#include <event/threadpool.h>
#include <netutil/dump.h>
#include <event/states.h>
#include <sys/syscall.h>   //syscall    
#include <event/event.h>   //event_ip_handle

static void* gather_thread(void* state);
static void ip_gather(void* recvd_msg);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
KAVLL_INIT(Mseg, Mseg, head, seg_cmp)
#pragma GCC diagnostic pop

errval_t gather_init(
    IP_gatherer* gather, size_t queue_size, size_t id
) {
    errval_t err = SYS_ERR_OK;
    
    // 1. Initialize the message queue
    BQelem * queue_elements = calloc(queue_size, sizeof(BQelem));
    err = bdqueue_init(&gather->msg_queue, queue_elements, queue_size);
    if (err_is_fail(err)) {
        IP_FATAL("Can't Initialize the queues for TCP messages, TODO: free the memory");
        return SYS_ERR_INIT_FAIL;
    }
        
    // 2. Initialize the semaphore for senders
    if (sem_init(&gather->msg_come, 0, 0) != 0) {
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
        return SYS_ERR_INIT_FAIL;
    }
    
    return err;
}

void gather_destroy(
    IP_gatherer* gather
) {
    assert(gather);

    bdqueue_destroy(&gather->msg_queue);
    sem_destroy(&gather->msg_come);
    pthread_cancel(gather->self);
    
    IP_ERR("TODO: free the memory");
    // kavll_free(Mseg, head, gather->seg, free);
    free(gather);
}

static void* gather_thread(void* state) {
    LocalState* local = state; assert(local);
    local->my_pid = syscall(SYS_gettid);
    set_local_state(local);

    IP_gatherer* gather = local->my_state; assert(gather);
    // TODO: remove my_id from local state
    // int my_id = local->my_id;  
    
    CORES_SYNC_BARRIER;
    IP_NOTE("%s started with pid %d", local->my_name, local->my_pid);
    
    IP_recv* msg = NULL;
    
    while (true)
    {
        if (debdqueue(&gather->msg_queue, NULL, (void**)&msg) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&gather->msg_come);
        } else {
            assert(msg);
            ip_gather(msg);
            free(msg);
            msg = NULL;
        }
    }
}

void drop_message(void* message) {
    IP_message* msg = message; assert(msg);
    IP_gatherer* gather = msg->gatherer; assert(gather);

    ip_msg_key_t msg_key = IP_MSG_KEY(msg->src_ip, msg->id);
    khint64_t key = kh_get(ip_msg, gather->recv_messages, msg_key);

    if (key == kh_end(gather->recv_messages))
        USER_PANIC("The message doesn't exist in hash table before we delete it!");

    // Delete the message from the hash table
    kh_del(ip_msg, gather->recv_messages, msg_key);

    if (msg->whole.size != msg->whole.recvd)
        IP_WARN("We drop a message that is not complete, size: %d, received: %d", msg->whole.size, msg->whole.recvd);

    Mseg_itr_t seg_itr = { 0 };
    Mseg_itr_first(msg->whole.seg, &seg_itr); // Initialize the iterator

    uint16_t offset = 0;
    while (kavll_at(&seg_itr))
    {
        const Mseg *node = kavll_at(&seg_itr);
        assert(offset == node->offset);   

        free_buffer(node->buf);

        // Move to the next node
        offset += node->buf.valid_size;
        Mseg_itr_next(&seg_itr);
    }

    free(message);
}

static Buffer segment_assemble(IP_message* msg) {
    assert(msg && msg->whole.seg);
    
    uint8_t* all_data = malloc(msg->whole.size); assert(all_data);
    Mseg_itr_t seg_itr = { 0 };
    Mseg_itr_first(msg->whole.seg, &seg_itr); // Initialize the iterator

    uint16_t offset = 0;
    while (kavll_at(&seg_itr))
    {
        const Mseg *node = kavll_at(&seg_itr);
        assert(offset == node->offset);   

        memcpy(all_data + offset, node->buf.data, node->buf.valid_size);
        free_buffer(node->buf);

        // Move to the next node
        offset += node->buf.valid_size;
        Mseg_itr_next(&seg_itr);
    }

    kavll_free(Mseg, head, msg->whole.seg, free);

    return (Buffer) {
        .data       = all_data,
        .from_hdr   = 0,
        .valid_size = msg->whole.size,
        .whole_size = msg->whole.size,
        .from_pool  = false,
        .mempool    = NULL,
    };
}

/**
 * @brief Checks the status of an IP message, drops it if TTL expired, or processes it if complete.
 *        This functions is called in a periodic event
 * 
 * @param message Pointer to the IP_recv structure to be checked.
 */
void check_recvd_message(void* message) {
    IP_VERBOSE("Checking a message");
    errval_t err = SYS_ERR_OK;
    IP_message* msg = message; assert(msg);
    IP_gatherer* gather = msg->gatherer; assert(gather);

    /// Also modified in ip_assemble(), be careful of global states
    msg->times_to_live *= 1.5;
    if (msg->times_to_live >= IP_GIVEUP_RECV_US)
    {
        drop_message(msg);
    }
    else
    {
        if (msg->whole.recvd == msg->whole.size) { // We can process the package now
            cancel_timer_task(msg->timer);

            // We don't need to care about duplicate segment here, they are deal in ip_assemble
            IP_DEBUG("We spliced an IP message of size %d, ttl: %d, now let's process it", msg->whole.size, msg->times_to_live);

            Buffer buf = segment_assemble(msg);
            IP_handle* handle = malloc(sizeof(IP_handle)); assert(handle);
            *handle = (IP_handle) {
                .ip     = msg->gatherer->ip,
                .proto  = msg->proto,
                .src_ip = msg->src_ip,
                .buf    = buf,
            };
            err =  submit_task(MK_NORM_TASK(event_ip_handle, (void*)handle));
            if (err_is_fail(err)) {
                DEBUG_ERR(err, "We assemble an IP message, but can't submit it as an event, will drop it");
                free_buffer(buf);
                free(handle);
            }
        }
        else
        {
            IP_VERBOSE("Done Checking a message, ttl: %d ms, whole size: %d, received %d", msg->times_to_live / 1000, msg->whole.size, msg->whole.recvd);
            Task task_for_myself = MK_TASK(&gather->msg_queue, &gather->msg_come, check_recvd_message, (void*)msg);
            msg->timer = submit_delayed_task(MK_DELAY_TASK(msg->times_to_live, drop_message, task_for_myself));
            submit_delayed_task(MK_DELAY_TASK(msg->times_to_live, drop_message, MK_NORM_TASK(check_recvd_message, (void*)msg)));
        }
    }
}

static void ip_gather(void* recvd_msg)
{
    IP_message *recv = recvd_msg;  assert(recv);
    IP_gatherer*gather = recv->gatherer; assert(gather);

    IP_message* msg = NULL;
    ip_msg_key_t msg_key = IP_MSG_KEY(((IP_message*)recvd_msg)->src_ip, ((IP_message*)recvd_msg)->id);

    uint16_t recvd_size = msg->seg.buf.valid_size;
    uint16_t offset     = msg->seg.offset;
    bool     no_frag    = msg->seg.no_frag;
    bool     more_frag  = msg->seg.more_frag;
    Buffer   buf        = msg->seg.buf;

    khint64_t key = kh_get(ip_msg, gather->recv_messages, msg_key);
    // Try to find if it already exists
    if (key == kh_end(gather->recv_messages)) {  // This message doesn't exist in the hash table of the gatherer

        msg = recv;     // We can directly use the received message

        msg->whole.recvd = recvd_size;
        msg->whole.size  = SIZE_DONT_KNOW;  // We don't know the size util the last packet arrives
        // Root of AVL tree
        msg->whole.seg = malloc(sizeof(Mseg)); assert(msg->whole.seg);
        msg->whole.seg->offset = offset;
        msg->whole.seg->buf    = buf;

        int ret;
        key = kh_put(ip_msg, gather->recv_messages, msg_key, &ret); 
        switch (ret) {
        case -1:    // The operation failed
            USER_PANIC("Can't add a new message with seqno: %d to hash table", msg->id);
        case 1:     // the bucket is empty 
        case 2:     // the element in the bucket has been deleted 
        case 0: 
            break;
        default:    USER_PANIC("Can't be this case: %d", ret);
        }
        // Set the value of key
        kh_value(gather->recv_messages, key) = msg;
        
    } else {

        msg = kh_val(gather->recv_messages, key); assert(msg);
        assert(recv->proto == msg->proto);

        ///ALARM: global state, also modified in check_recvd_message()
        msg->times_to_live /= 1.5;  // We got 1 more packet, wait less time

        Mseg *seg = malloc(sizeof(Mseg)); assert(seg);
        msg->whole.seg->offset = offset;
        msg->whole.seg->buf    = buf;

        if (seg != Mseg_insert(&msg->whole.seg, seg)) { // Already exists !
            IP_ERR("We have duplicate IP message segmentation with offset: %d", seg->offset);
            free_buffer(seg->buf);
            free(seg);
            return;
        }
    }

    // If this the laset packet, we now know the final size
    if (more_frag == false) {
        assert(no_frag == false);
        assert(msg->whole.size == SIZE_DONT_KNOW);
        msg->whole.size = offset + recvd_size;
    }
    
    // It's guaranteed that the message is not duplicated
    msg->whole.recvd += recvd_size;

    // If the message is complete, we trigger the check message,
    if (msg->whole.recvd == msg->whole.size)
    {
        IP_DEBUG("We have received a complete IP message of size %d, now let's process it", msg->whole.size);
        check_recvd_message((void*) recv);
    }
    else
    {
        IP_DEBUG("We have received %d bytes of a message of size %d, now let's wait for %d ms", msg->whole.recvd, msg->whole.size, msg->times_to_live / 1000);
        Task task_for_myself = MK_TASK(&gather->msg_queue, &gather->msg_come, check_recvd_message, (void*)msg);
        msg->timer = submit_delayed_task(MK_DELAY_TASK(msg->times_to_live, drop_message, task_for_myself));
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
