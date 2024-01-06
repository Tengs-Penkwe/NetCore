#include <netutil/icmpv6.h>
#include <netstack/ndp.h>
#include <netstack/ip.h>
#include <netutil/htons.h>
#include <netutil/dump.h>
#include <event/event.h>
#include <event/threadpool.h>

errval_t ndp_lookup_mac(
    ICMP* icmp, ipv6_addr_t dst_ip, mac_addr* ret_mac
) {
    errval_t err = SYS_ERR_OK;
    assert(icmp && ret_mac);

    assert(!ipv6_is_multicast(dst_ip) && dst_ip != 0);

    void* macaddr_as_pointer = NULL;
    err = hash_get_by_key(&icmp->hosts, &dst_ip, &macaddr_as_pointer);
    DEBUG_FAIL_PUSH(err, NET_ERR_NO_MAC_ADDRESS, "Can't find the MAC address of given IPv6 address");

    assert(macaddr_as_pointer);
    *ret_mac = voidptr2mac(macaddr_as_pointer);

    assert(!(maccmp(*ret_mac, MAC_NULL) || maccmp(*ret_mac, MAC_BROADCAST)));
    return err;
}

void ndp_register(
    ICMP* icmp, ipv6_addr_t ip, mac_addr mac
) {
    assert(icmp); errval_t err = SYS_ERR_OK;

    // in 64-bit machine, a pointer isn't big enough to store the ipv6_addr_t, so we allocate a heap memory to store it
    ipv6_addr_t *ip_heap  = malloc(sizeof(ipv6_addr_t));
    *ip_heap              = ip;

    // in 64-bit machine, a pointer is big enough to store the mac_addr, so we cast the value mac to a void pointer, not its address
    void* macaddr_as_pointer = NULL;
    memcpy(&macaddr_as_pointer, &mac, sizeof(mac_addr));

    err = hash_insert(&icmp->hosts, (void*)ip_heap, macaddr_as_pointer);
    if (err_no(err) == EVENT_HASH_OVERWRITE_ON_INSERT) {
        NDP_INFO("The IP-MAC pair already exists, update it");
    } else if (err_is_fail(err)) {
        DEBUG_ERR(err, "NDP can't insert this IP-MAC to the hash table !");
    }
}

errval_t ndp_marshal(
    ICMP* icmp, ipv6_addr_t dst_ip, uint8_t type, uint8_t code, Buffer buf
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    assert(icmp);

    switch (type) {
    case ICMPv6_NSA:
        NDP_VERBOSE("Sending a Neighbor Solicitation !");

        buffer_reclaim_ptr(&buf, NDP_HEADER_RESERVE + sizeof(struct ndp_neighbor_advertisement) + sizeof(struct ndp_option) + sizeof(mac_addr), 0);

        buffer_sub_ptr(&buf, sizeof(struct ndp_option) + sizeof(mac_addr));
        struct ndp_option *my_option = (struct ndp_option *)buf.data;
        *my_option = (struct ndp_option) {
            .type   = NDP_OPTION_TARGET_LINK_LAYER_ADDRESS,
            .length = 1,
        };
        memcpy(my_option->data, &icmp->my_mac, sizeof(mac_addr));

        buffer_sub_ptr(&buf, sizeof(struct ndp_neighbor_advertisement));
        struct ndp_neighbor_advertisement *nsl = (struct ndp_neighbor_advertisement *)buf.data;
        *nsl = (struct ndp_neighbor_advertisement) {
            .flags_reserved   = htonl(NDP_NSA_RSO(false, true, false)),
            // TODO: When need us to override the MAC address ?
            .from_addr        = hton16(icmp->ip->my_ipv6),
        };

        err = icmpv6_marshal(icmp, dst_ip, ICMPv6_NSA, code, buf);  

        break;
    default:
        NDP_ERR("Unknown NDP type: %d\n", type);
        return NET_ERR_NDP_INVALID_OPTION;
    }

    return err;
}

errval_t ndp_neighbor_solicitation(
    ICMP* icmp, ipv6_addr_t src_ip, uint8_t code, Buffer buf
) {
    assert(icmp); errval_t err = SYS_ERR_NOT_IMPLEMENTED;

    // 1. Check the code
    if (code != 0) return NET_ERR_ICMPv6_WRONG_CODE;

    // 2. Read the NSL header
    struct ndp_neighbor_solicitation *nsl = (struct ndp_neighbor_solicitation *)buf.data;
    buffer_add_ptr(&buf, sizeof(struct ndp_neighbor_solicitation));

    // 3. Check the destination IP
    ipv6_addr_t dst_ip = ntoh16(nsl->to_addr);
    if (dst_ip != icmp->ip->my_ipv6 && !ipv6_is_multicast(dst_ip)) return NET_ERR_NDP_WRONG_DESTINATION;

    // 4. Read the options
    struct ndp_option *option = (struct ndp_option *)buf.data;
    buffer_add_ptr(&buf, option->length * 8);

    if (buf.valid_size > 0) {
        NDP_ERR("There are still %d bytes left in the NSL, But we only support 1 option field", buf.valid_size);
    }

    switch (option->type) {
    case NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS:
        ndp_register(icmp, src_ip, ntoh6(mem2mac(option->data)));

        NDP_marshal *marshal = malloc(sizeof(NDP_marshal));
        *marshal = (NDP_marshal) {
            .icmp   = icmp,
            .dst_ip = src_ip,
            .type   = ICMPv6_NSA,
            .code   = 0,
            .buf    = buf,
        };

        err = submit_task(MK_NORM_TASK(event_ndp_marshal, (void *)marshal));
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "Can't submit NDP the task");
            assert(!err_is_throw(err));
        } else {
            err = NET_THROW_SUBMIT_EVENT;  // Tell the event caller to free the buffer in upper layer
        }
        return err;
    case NDP_OPTION_TARGET_LINK_LAYER_ADDRESS:
    case NDP_OPTION_PREFIX_INFORMATION:
    case NDP_OPTION_REDIRECTED_HEADER:
    case NDP_OPTION_MTU:
    default: NDP_ERR("Invalid NDP option type: %d inside an NSL", option->type);
        return NET_ERR_NDP_INVALID_OPTION;
    }
}