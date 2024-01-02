#include <netutil/ndp.h>
#include <netstack/ndp.h>
#include <netstack/ip.h>

errval_t ndp_init(
    NDP* ndp, Ethernet* ether, ipv6_addr_t my_ipv6
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    ndp->ether   = ether;
    ndp->my_ipv6 = my_ipv6;

    return err;
}

void ndp_destroy(
    NDP* ndp
) {
    assert(ndp);

    NDP_ERR("Not implemented yet");
}

errval_t ndp_lookup_mac(
    NDP* ndp, ipv6_addr_t dst_ip, mac_addr* ret_mac
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    assert(ndp && ret_mac);
    
    (void)dst_ip;


    return err;
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