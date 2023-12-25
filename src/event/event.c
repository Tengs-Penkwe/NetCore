#include <common.h>
#include <netstack/ethernet.h>
#include <event/event.h>
#include <device/device.h>      //DEVICE_HEADER_RESERVE

#include <netutil/dump.h>

void frame_unmarshal(void* frame) {
    errval_t err;
    assert(frame);

    Frame* fr = frame;
    Ethernet *ether = fr->ether;
    uint8_t  *data  = fr->data;
    size_t    size  = fr->size;

    printf("Read %d bytes from TAP device\n", size);
    dump_packet_info(data);
    err = ethernet_unmarshal(ether, data, size);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "We meet an error when processing this frame, but the process continue");
    }
    printf("========================================\n");
    
    free(data - DEVICE_HEADER_RESERVE);
    free(frame);
}