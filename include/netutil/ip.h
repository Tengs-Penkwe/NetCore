#ifndef _IP_H_
#define _IP_H_

#include <common.h>

#define IP_RF 0x8000U        /* reserved fragment flag */
#define IP_DF 0x4000U        /* dont fragment flag */
#define IP_MF 0x2000U        /* more fragments flag */
#define IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */
#define IP_LEN_MIN      20        /* minimum size of an ip header*/
#define IP_LEN_MAX      0x10000   /* maximum size of an ip header*/
#define IP_PROTO_ICMP    1
#define IP_PROTO_IGMP    2
#define IP_PROTO_UDP     17
#define IP_PROTO_UDPLITE 136
#define IP_PROTO_TCP     6

#define OFFSET_RF_SET(offset, rf) (offset |= (((uint16_t)rf) << 15))
#define OFFSET_DF_SET(offset, df) (offset |= (((uint16_t)df) << 14))
#define OFFSET_MF_SET(offset, mf) (offset |= (((uint16_t)mf) << 13))

typedef uint32_t ip_addr_t;
#define MK_IP(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

#define IPH_V(hdr)  ((hdr)->version)
#define IPH_HL(hdr) ((hdr)->ihl * 4)
#define IPH_VHL_SET(hdr, v, hl) (hdr)->v_hl = (((v) << 4) | (hl))

struct ip_hdr {
  /* header length */
  uint8_t ihl:4 ;
  /* version  */
  uint8_t version:4 ;
  /* type of service */
  uint8_t tos;
  /* total length */
  uint16_t len;
  /* identification */
  uint16_t id;
  /* fragment offset field */
  uint16_t offset;
  /* time to live */
  uint8_t ttl;
  /* protocol*/
  uint8_t proto;
  /* checksum */
  uint16_t chksum;
  /* source and destination IP addresses */
  ip_addr_t src;
  ip_addr_t dest; 
} __attribute__((__packed__));

#endif // _IP_H_
