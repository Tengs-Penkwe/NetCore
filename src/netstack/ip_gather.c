#include "ip_gather.h"
#include <event/timer.h>
#include <event/threadpool.h>
#include <netutil/dump.h>

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wmissing-prototypes"
// KAVLL_INIT(Mseg, Mseg, head, seg_cmp)
// #pragma GCC diagnostic pop

// void drop_message(void* message) {
//     IP_recv* msg = message; assert(msg);
//     IP* ip = msg->ip; assert(ip);

//     ip_msg_key_t msg_key = MSG_KEY(msg->src_ip, msg->id);
//     khint64_t key = kh_get(ip_recv, ip->recv_messages, msg_key);

//     if (key == kh_end(ip->recv_messages) && msg->alloc_size != 0)
//         USER_PANIC("The message doesn't exist in hash table before we delete it!");

//     if (msg->alloc_size != 0) { // Means there was assembling process
//         // Delete the message from the hash table
//         pthread_mutex_lock(&ip->mutex);
//         kh_del(ip_recv, ip->recv_messages, msg_key);
//         pthread_mutex_unlock(&ip->mutex);
//         IP_NOTE("Deleted a message from hash table");

//         // Free all segments
//         assert(msg->seg);
//         kavll_free(Mseg, head, msg->seg, free);
//         msg->seg = NULL;

//         USER_PANIC("TODO: free all the data");

//         // Note: it doesn't matter if we use recvd_size or sent.size since we used the union
//         assert(msg->whole_size && msg->recvd_size);
//         // Free the allocated data:
//         free(msg->data);
//         // if message come here directly, data should be free'd in lower module

//         // Destroy mutex
//         pthread_mutex_destroy(&msg->mutex);
//     }

//     assert(msg->seg == NULL);
//     // Free the message itself and set it to NULL
//     free(message);
//     message = NULL;
// }

// /**
//  * @brief Checks the status of an IP message, drops it if TTL expired, or processes it if complete.
//  *        This functions is called in a periodic event
//  * 
//  * @param message Pointer to the IP_recv structure to be checked.
//  */
// void check_recvd_message(void* message) {
//     IP_VERBOSE("Checking a message");
//     errval_t err;
//     IP_recv* msg = message; assert(msg);

//     /// Also modified in ip_assemble(), be careful of global states
//     msg->times_to_live *= 1.5;
//     if (msg->times_to_live >= IP_GIVEUP_RECV_US)
//     {
//         drop_message(msg);
//     }
//     else
//     {
//         if (msg->recvd_size == msg->whole_size) { // We can process the package now
//             // We don't need to care about duplicate segment here, they are deal in ip_assemble
//             IP_DEBUG("We spliced an IP message of size %d, ttl: %d, now let's process it", msg->whole_size, msg->times_to_live);

//             err = ip_handle(msg);
//             if (err_not_ok(err)) { DEBUG_ERR(err, "We meet an error when handling an IP message"); }
//             drop_message(msg);
//         }
//         else
//         {
//             submit_delayed_task(MK_DELAY_TASK(msg->times_to_live, drop_message MK_TASK(check_recvd_message, (void*)msg)));
//             IP_VERBOSE("Done Checking a message, ttl: %d, whole size: %p", msg->times_to_live, msg->whole_size);
//         }
//     }
// }

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
        err = icmp_unmarshal(msg->ip->icmp, msg->src_ip, msg->buf);
        DEBUG_FAIL_RETURN(err, "Error when unmarshalling an ICMP message");
        return err;
    case IP_PROTO_UDP:
        IP_VERBOSE("Received a UDP packet");
        err = udp_unmarshal(msg->ip->udp, msg->src_ip, msg->buf);
        DEBUG_FAIL_RETURN(err, "Error when unmarshalling an UDP message");
        return err;
    case IP_PROTO_IGMP:
    case IP_PROTO_UDPLITE:
        LOG_ERR("Unsupported Protocal");
        return SYS_ERR_NOT_IMPLEMENTED;
    case IP_PROTO_TCP:
        IP_VERBOSE("Received a TCP packet");
        // err = tcp_unmarshal(msg->ip->tcp, msg->src_ip, msg->, msg->recvd_size);
        // DEBUG_FAIL_RETURN(err, "Error when unmarshalling an TCP message");
        return SYS_ERR_NOT_IMPLEMENTED;
    default:
        LOG_ERR("Unknown packet type for IPv4: %p", msg->proto);
        return NET_ERR_IPv4_WRONG_PROTOCOL;
    }
}
