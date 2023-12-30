#include <common.h>
#include <netstack/ethernet.h>
#include <event/event.h>

void event_ether_unmarshal(void* unmarshal) {

    Ether_unmarshal* frame = unmarshal; assert(frame);
    errval_t err = ethernet_unmarshal(frame->ether, frame->buf);
    switch (err_no(err))
    {
    case NET_OK_TCP_ENQUEUE:
    {
        EVENT_INFO("A TCP message is successfully enqueued, Can't free the buffer now");
        free(frame);
        break;
    }
    case NET_OK_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        free(frame);
        break;
    }
    case NET_OK_IPv4_SEG_LATER_FREE:
    {
        EVENT_INFO("A Segmented IP message received, Can't free the buffer now");
        free(frame);
        break;
    }
    case NET_ERR_TCP_QUEUE_FULL:
    {
        assert(err_pop(err) == EVENT_ENQUEUE_FULL);
        EVENT_WARN("This should be a TCP message that has its queue full, drop it");
        free_ether_unmarshal(frame);
        break;
    }
    case NET_ERR_ETHER_WRONG_MAC:
    case NET_ERR_ETHER_NO_MAC:
        DEBUG_ERR(err, "A known error happend, the process continue");
        break;
    case SYS_ERR_OK:
        free_ether_unmarshal(frame);
        break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}

void event_arp_marshal(void* send) {
    errval_t err; assert(send);

    ARP_marshal* marshal = send;

    err = arp_marshal(marshal->arp, marshal->opration, marshal->dst_ip, marshal->dst_mac, marshal->buf);
    switch (err_no(err))
    {
    case SYS_ERR_OK:
        free_arp_marshal(marshal);
        break;
    // case 
    //     break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}

void event_icmp_marshal(void* send) {
    errval_t err; assert(send);

    ICMP_marshal* marshal = send;

    err = icmp_marshal(marshal->icmp, marshal->dst_ip, marshal->type, marshal->code, marshal->field, marshal->buf);
    switch (err_no(err))
    {
    case NET_OK_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        free(send);
        break;
    }
    case SYS_ERR_OK:
        free_icmp_marshal(marshal);
        break;
    // case 
    //     break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}

void event_ip_handle(void* recv) {
    errval_t err; assert(recv);

    IP_handle* handle = recv;

    err = ip_handle(handle->ip, handle->proto, handle->src_ip, handle->buf);
    switch (err_no(err))
    {
    case NET_OK_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        free(handle);
        break;
    }
    case SYS_ERR_OK:
        free(handle);
        break;
    // case 
    //     break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}