#include <common.h>
#include <netstack/ethernet.h>
#include <event/event.h>

#include <netutil/dump.h>

void frame_receive(void* packet) {
    LOG_ERR("HERE");
    dump_packet_info(packet);
}