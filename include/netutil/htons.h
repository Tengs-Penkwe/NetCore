#ifndef _HTONS_H_
#define _HTONS_H_

#include <common.h>

/*
 * Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#ifndef htons
#define htons(x) lwip_htons(x)
#endif

#ifndef ntohs
#define ntohs(x) lwip_ntohs(x)
#endif

#ifndef htonl
#define htonl(x) lwip_htonl(x)
#endif

#ifndef ntohl
#define ntohl(x) lwip_ntohl(x)
#endif

uint16_t lwip_htons(uint16_t x);
uint16_t lwip_ntohs(uint16_t x);
uint32_t lwip_htonl(uint32_t x);
uint32_t lwip_ntohl(uint32_t x);

#include <netutil/etharp.h>
struct eth_addr hton6(mac_addr mac);
struct eth_addr ntoh6(mac_addr mac);

#include <netutil/ip.h>
ipv6_addr_t hton16(ipv6_addr_t ip);
ipv6_addr_t ntoh16(ipv6_addr_t ip);

#endif //_HTONS_H_
