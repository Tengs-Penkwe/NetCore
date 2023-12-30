#include <common.h>
#include <netstack/ethernet.h>
#include <event/event.h>

void event_ether_unmarshal(void* unmarshal) {
    
    // TODO: copy the argument to stack, and free the argument
    assert(unmarshal);
    Ether_unmarshal frame = *(Ether_unmarshal*) unmarshal; 
    free(unmarshal);

    errval_t err = ethernet_unmarshal(frame.ether, frame.buf);
    switch (err_no(err))
    {
    case NET_THROW_TCP_ENQUEUE:
    {
        EVENT_INFO("A TCP message is successfully enqueued, Can't free the buffer now");
        break;
    }
    case NET_THROW_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        break;
    }
    case NET_THROW_IPv4_SEG:
    {
        EVENT_INFO("A Segmented IP message received, Can't free the buffer now");
        break;
    }
    case NET_ERR_TCP_QUEUE_FULL:
    {
        assert(err_pop(err) == EVENT_ENQUEUE_FULL);
        EVENT_WARN("This should be a TCP message that has its queue full, drop it");
        free_buffer(frame.buf);
        break;
    }
    case NET_ERR_ETHER_WRONG_MAC:
    case NET_ERR_ETHER_NO_MAC:
        free_buffer(frame.buf);
        DEBUG_ERR(err, "A known error happend, the process continue");
        break;
    case SYS_ERR_OK:
        free_buffer(frame.buf);
        break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}

void event_arp_marshal(void* send) {
    errval_t err; assert(send);

    // TODO: copy the argument to stack, and free the argument
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

    // TODO: copy the argument to stack, and free the argument
    ICMP_marshal* marshal = send;

    err = icmp_marshal(marshal->icmp, marshal->dst_ip, marshal->type, marshal->code, marshal->field, marshal->buf);
    switch (err_no(err))
    {
    case NET_THROW_SUBMIT_EVENT:
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

void event_ip_gather(void* recvd_segment) {
    errval_t err; assert(recv);

    IP_segment seg = *(IP_segment*) recvd_segment;
    free(recvd_segment);

    err = ip_gather(&seg);
    switch (err_no(err))
    {
    case NET_THROW_IPv4_ASSEMBLE: {
        EVENT_INFO("An IP message is successfully assembled, Let the handler free the buffer");
        break;
    }
    case NET_THROW_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        break;
    }
    case NET_ERR_IPv4_DUPLITCATE_SEG:
    {
        EVENT_INFO("A duplicated IP message received, free the buffer now");
        free_buffer(seg.buf);
        break;
    }
    case SYS_ERR_OK:
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}

void event_ip_handle(void* recv) {
    errval_t err; assert(recv);

    IP_handle handle = *(IP_handle*) recv;
    free(recv);

    err = ip_handle(handle.ip, handle.proto, handle.src_ip, handle.buf);
    switch (err_no(err))
    {
    case NET_THROW_SUBMIT_EVENT:
    {
        EVENT_INFO("An Event is submitted, and the buffer is re-used, can't free now");
        break;
    }
    case SYS_ERR_OK:
        free_buffer(handle.buf); 
        break;
    // case 
    //     break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
}