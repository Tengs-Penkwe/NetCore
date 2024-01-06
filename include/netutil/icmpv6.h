#ifndef __ICMP_v6_H__
#define __ICMP_v6_H__

#include <stdint.h>
#include "ip.h"     // for ipv6_addr_t

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// ICMPv6 ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define ICMPv6_DUR   1   /* destination unreachable */
enum icmpv6_dur_type {
    ICMPv6_DUR_NO_ROUTE      = 0, /* no route to destination */
    ICMPv6_DUR_COMM_PROHIB   = 1, /* communication with destination administratively prohibited */
    ICMPv6_DUR_BEYOND_SCOPE  = 2, /* beyond scope of source address */
    ICMPv6_DUR_ADDR_UNREACH  = 3, /* address unreachable */
    ICMPv6_DUR_PORT_UNREACH  = 4, /* port unreachable */
    ICMPv6_DUR_SRC_ADDR_FAIL = 5, /* source address failed ingress/egress policy */
    ICMPv6_DUR_REJECT_ROUTE  = 6, /* reject route to destination */
};

#define ICMPv6_PTB   2   /* packet too big */
#define ICMPv6_TE    3   /* time exceeded */
#define ICMPv6_PP    4   /* parameter problem */
#define ICMPv6_ECHO  128 /* echo request */
#define ICMPv6_ER    129 /* echo reply */
#define ICMPv6_MLQ   130 /* multicast listener query */
#define ICMPv6_MLR   131 /* multicast listener report */
#define ICMPv6_MLD   132 /* multicast listener done */

/* Neighbor Discovery Protocol (NDP) */
#include "ndp.h" 
#define ICMPv6_RSS   133 /* NDP router solicitation */
#define ICMPv6_RSA   134 /* NDP router advertisement */
#define ICMPv6_NSL   135 /* NDP neighbor solicitation */
#define ICMPv6_NSA   136 /* NDP neighbor advertisement */
#define ICMPv6_RR    137 /* NDP redirect */

/* Multicast Listener Discovery (MLD) */
#include "mldv2.h"
#include "mldv2.h"
#define ICMPv6_MLPv2 143   /* Multicast Listener Report V2 */

/* Secure Neighbor Discovery */

/* Multicast router discovery */

/* Function declarations */
// Functions to parse and process NDP messages would be defined here.

#endif // __NDP_H__
