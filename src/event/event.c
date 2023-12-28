#include <common.h>
#include <netstack/ethernet.h>
#include <event/event.h>

void frame_unmarshal(void* frame) {
    errval_t err;
    assert(frame);

    Frame* fr = frame;
    Ethernet *ether = fr->ether;
    uint8_t  *data  = fr->data;
    size_t    size  = fr->size;

    err = ethernet_unmarshal(ether, data, size);
    switch (err_no(err))
    {
    case NET_OK_IPv4_SEG_LATER_FREE: {
        // We need to keep the buffer for later assembling DONT free it here
        free(frame);
        break;
    }
    case NET_ERR_TCP_QUEUE_FULL:
    {
        assert(err_pop(err) == EVENT_ENQUEUE_FULL);
        EVENT_WARN("This should be a TCP message that has its queue full, drop it");
        frame_free(frame);
        break;
    }
    case NET_OK_TCP_ENQUEUE:
    {
        EVENT_INFO("A TCP message is successfully enqueued, Can't free the buffer now");
        free(frame);
        break;
    }
    default:
        if (err_is_fail(err))
            DEBUG_ERR(err, "An error happened during ethernet_unmarshal(), but let's continue");
    }
}

void send_arp_request(void* arp_request) {
    errval_t err;
    assert(arp_request);
    ARP_Request* req = arp_request;

    err = arp_send(req->arp, ARP_OP_REQ, req->dst_ip, MAC_BROADCAST);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Can't send an ARP request in event");
    }
    free(arp_request);
}