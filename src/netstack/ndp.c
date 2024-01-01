#include <netutil/ndp.h>
#include <netstack/ndp.h>
#include <netstack/ipv6.h>

errval_t ndp_init(
    NDP* ndp, ipv6_addr_t my_ip, IPv6* ip
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    ndp->ip6 = ip;
    ndp->my_ip = my_ip;

    return err;
}

void ndp_destroy(
    NDP* ndp
) {
    assert(ndp);

    NDP_ERR("Not implemented yet");
}

__attribute_maybe_unused__
static errval_t ndp_neighbor_solicit(
    NDP* ndp, ipv6_addr_t dst_ip, Buffer buf
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    assert(ndp);
    
    (void)dst_ip;
    (void)buf;
    
    // errval_t ipv6_marsh_err = ipv6_marshal(
    //     ndp->ip, dst_ip, buf
    // );
    
    return err;
}

errval_t ndp_unmarshal(
    NDP* ndp, Buffer buf
) {
    errval_t err = SYS_ERR_OK;
    assert(ndp);
    
    struct ndp_hdr* packet = (struct ndp_hdr*)buf.data;
    
    uint8_t code = packet->code;
    if (code != 0) {
        NDP_ERR("Invalid NDP code %d", code)
        return NET_ERR_NDP_WRONG_FIELD;
    }

    uint8_t type = packet->type;
    switch (type)
    {
    case NDP_ROUTER_SOLICITATION:
        break;
    case NDP_NEIGHBOR_SOLICITATION:
        // err = ndp_neighbor_solicit(
        //     ndp, packet->target_addr, buf
        // );
    case NDP_ROUTER_ADVERTISEMENT:
    case NDP_NEIGHBOR_ADVERTISEMENT:
    case NDP_REDIRECT:
    default: USER_PANIC("Unknown NDP type %d", type);
    }
    NDP_FATAL("Not implemented yet");
    
    return err;
}