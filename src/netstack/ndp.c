#include <netutil/ndp.h>
#include <netstack/ndp.h>

errval_t ndp_init(
    NDP* ndp, ipv6_addr_t my_ip
) {
    errval_t err = SYS_ERR_OK;

    ndp->my_ip = my_ip;

    return err;
}

void ndp_destroy(
    NDP* ndp
) {
    assert(ndp);

    NDP_ERR("Not implemented yet");
}

