#include <netutil/icmpv6.h>
#include <netstack/ndp.h>
#include <netstack/ip.h>
#include <netutil/htons.h>

errval_t ndp_lookup_mac(
    ICMP* icmp, ipv6_addr_t dst_ip, mac_addr* ret_mac
) {
    errval_t err = SYS_ERR_NOT_IMPLEMENTED;
    assert(icmp && ret_mac);
    
    (void)dst_ip;


    return err;
}

void ndp_register(
    ICMP* icmp, ipv6_addr_t ip, mac_addr mac
) {
    assert(icmp); errval_t err = SYS_ERR_OK;
    ipv6_addr_t *ip_heap  = malloc(sizeof(ipv6_addr_t));
    mac_addr    *mac_heap = malloc(sizeof(mac_addr));
    *ip_heap              = ip;
    *mac_heap             = mac;

    err = hash_insert(&icmp->hosts, (void*)ip_heap, (void*)mac_heap, false);
    if (err_no(err) == EVENT_HASH_EXIST_ON_INSERT) {
        NDP_INFO("The IP-MAC pair already exists");
    } else if (err_is_fail(err)) {
        DEBUG_ERR(err, "NDP can't insert this IP-MAC to the hash table !");
    }
}
