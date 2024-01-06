/**
 * @file mldv2.h
 * @brief Header file for MLDv2 (Multicast Listener Discovery Version 2) protocol.
 * 
 * This header file defines the structure and format of MLDv2 messages as specified in RFC 3810.
 * MLDv2 is a protocol used by IPv6 routers and hosts to discover and manage multicast group memberships.
 * 
 * The MLDv2 message format consists of various fields including type, code, checksum, maximum response code,
 * reserved, multicast address, reserved, S (Supress Router-Side Processing), QRV (Querier's Robustness Variable),
 * QQIC (Querier's Query Interval Code), number of sources, and source addresses.
 * 
 * This header file is part of the TCP-IP project and can be found in the include/netutil directory.
 * 
 * @see https://datatracker.ietf.org/doc/html/rfc3810
 */

/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Type = 130   |      Code     |           Checksum            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Maximum Response Code      |           Reserved            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
*                                                               *
|                                                               |
*                       Multicast Address                       *
|                                                               |
*                                                               *
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Resv  |S| QRV |     QQIC      |     Number of Sources (N)     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [1]                      *
|                                                               |
*                                                               *
|                                                               |
+-                                                             -+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [2]                      *
|                                                               |
*                                                               *
|                                                               |
+-                              .                              -+
.                               .                               .
.                               .                               .
+-                                                             -+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [N]                      *
|                                                               |
*                                                               *
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#ifndef _MLDV2_H_
#define _MLDV2_H_

#include <stdint.h>
#include "ip.h"     // for ipv6_addr_t

struct mldv2_hdr {
    // ICMPv6 header
    // uint8_t type = 130 (MLDv2)
    // uint8_t code = 0
    // uint16_t checksum

    union { 
        struct {    // A special kind of half-precision float that doesn't follow IEEE 754, need special handling
            uint16_t sign   : 1;
            uint16_t exp    : 3;
            uint16_t mant   : 12;
        };
        uint16_t    u16_max_response_code;   // To be used for byte order conversion
    } __attribute__((__packed__)) ;

    uint16_t     reserved;

    ipv6_addr_t multicast_addr;
    
    union {
        struct {
            uint8_t resv      : 4;  // Another reserved field 
            uint8_t s_flag    : 1;  // Supress Router-Side Processing
            uint8_t qrv       : 3;  // Querier's Robustness Variable
        };
        uint8_t resv_s_qrv;         // Reserved, S, QRV
    } __attribute__((__packed__)) ;
    uint8_t     qqic;               // Querier's Query Interval Code

    uint16_t    num_sources;        
    ipv6_addr_t source_addr[];
} __attribute__((__packed__));

static_assert(sizeof(struct mldv2_hdr) == 24, "MLDv2 header size is not 24 bytes");

/*
The Maximum Response Code field specifies the maximum time allowed
    before sending a responding Report.  The actual time allowed, called
    the Maximum Response Delay, is represented in units of milliseconds,
    and is derived from the Maximum Response Code as follows:

    If Maximum Response Code < 32768,
        Maximum Response Delay = Maximum Response Code

    If Maximum Response Code >=32768, Maximum Response Code represents a
    floating-point value as follows:

    0 1 2 3 4 5 6 7 8 9 A B C D E F
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |1| exp |          mant         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Maximum Response Delay = (mant | 0x1000) << (exp+3)
*/

#include "htons.h"

static inline uint32_t mldv2_max_response_from_net_order(uint16_t hf16) {
    if (hf16 < 32768) {
        return ntohs(hf16);
    } else {
        uint16_t mant = ntohs(hf16) & 0x0FFF;
        uint16_t exp  = ntohs(hf16) >> 12 & 0x0007;
        return (mant | 0x1000) << (exp+3);
    }
}


/**
 * @brief Multicast Listener Report V2 (MLPv2) message.
 */

/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Type = 143   |    Reserved   |           Checksum            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Reserved            |Nr of Mcast Address Records (M)|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
.                                                               .
.                  Multicast Address Record [1]                 .
.                                                               .
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
.                                                               .
.                  Multicast Address Record [2]                 .
.                                                               .
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               .                               |
.                               .                               .
|                               .                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
.                                                               .
.                  Multicast Address Record [M]                 .
.                                                               .
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Each Multicast Address Record has the following internal format:

+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Record Type  |  Aux Data Len |     Number of Sources (N)     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
*                                                               *
|                                                               |
*                       Multicast Address                       *
|                                                               |
*                                                               *
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [1]                      *
|                                                               |
*                                                               *
|                                                               |
+-                                                             -+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [2]                      *
|                                                               |
*                                                               *
|                                                               |
+-                                                             -+
.                               .                               .
.                               .                               .
.                               .                               .
+-                                                             -+
|                                                               |
*                                                               *
|                                                               |
*                       Source Address [N]                      *
|                                                               |
*                                                               *
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
.                                                               .
.                         Auxiliary Data                        .
.                                                               .
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#include <stdint.h>
#include "ip.h"     // for ipv6_addr_t

/**
 * @struct mutlicast_address_record
 * @brief Represents a multicast address record in MLDv2 message.
 * 
 * Each multicast address record contains the following information:
 * - Record Type: Type of the record.
 * - Aux Data Len: Length of the auxiliary data.
 * - Number of Sources: Number of source addresses in the record.
 * - Multicast Address: IPv6 multicast address.
 * - Source Addresses: Array of IPv6 source addresses.
 * - Auxiliary Data: Additional data associated with the record.
 */
struct mutlicast_address_record {
    uint8_t     record_type;
    uint8_t     aux_data_len;
    uint16_t    num_sources;
    ipv6_addr_t multicast_addr;
    ipv6_addr_t source_addr[];
    // uint8_t     aux_data[];
} __attribute__((__packed__)) ;

static_assert(sizeof(struct mutlicast_address_record) == 20, "Multicast Address Record size is not 20 bytes");

/**
 * @struct mlpv2_hdr
 * @brief Represents the MLDv2 message header.
 * 
 * The MLDv2 message header contains the following information:
 * - Reserved: Reserved field.
 * - Number of Records: Number of multicast address records in the message.
 * - Multicast Address Records: Array of multicast address records.
 */
struct mlpv2_hdr {
    // uint8_t type = 143 (MLPv2)
    // uint8_t code = 0
    // uint16_t checksum

    uint16_t reserved;
    uint16_t num_records;

    // struct mutlicast_address_record records[];
} __attribute__((__packed__));

static_assert(sizeof(struct mlpv2_hdr) == 4, "MLDv2 header size is not 4 bytes");

#endif // _MLDV2_H_