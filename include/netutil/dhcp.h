#ifndef __DHCP_H__
#define __DHCP_H__

#include <stdint.h>

/* DHCP message types */
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7
#define DHCP_INFORM   8

typedef uint16_t dhcp_port_t;

/* Default DHCP client and server ports */
#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

/* DHCP packet structure */
struct dhcp_packet {
    uint8_t  op;         /* Message op code / message type */
    uint8_t  htype;      /* Hardware address type */
    uint8_t  hlen;       /* Hardware address length */
    uint8_t  hops;       /* Hops */
    uint32_t xid;        /* Transaction ID */
    uint16_t secs;       /* Seconds elapsed since client began address acquisition or renewal process */
    uint16_t flags;      /* Flags */
    uint32_t ciaddr;     /* Client IP address */
    uint32_t yiaddr;     /* 'Your' (client) IP address */
    uint32_t siaddr;     /* Next server IP address to use in bootstrap */
    uint32_t giaddr;     /* Relay agent IP address */
    uint8_t  chaddr[16]; /* Client hardware address */
    uint8_t  sname[64];  /* Server host name */
    uint8_t  file[128];  /* Boot file name */
    uint32_t magic;      /* Magic cookie */
    uint8_t  options[];  /* Optional parameters field */
} __attribute__((__packed__));

/* DHCP Options */
#define DHCP_OPTION_PAD                        0
#define DHCP_OPTION_SUBNET_MASK                1
#define DHCP_OPTION_TIME_OFFSET                2
#define DHCP_OPTION_ROUTER                     3
#define DHCP_OPTION_TIME_SERVER                4
#define DHCP_OPTION_NAME_SERVER                5
#define DHCP_OPTION_DOMAIN_NAME_SERVERS        6
#define DHCP_OPTION_LOG_SERVER                 7
#define DHCP_OPTION_COOKIE_SERVER              8
#define DHCP_OPTION_LPR_SERVER                 9
#define DHCP_OPTION_IMPRESS_SERVER             10
#define DHCP_OPTION_RESOURCE_LOCATION_SERVER   11
#define DHCP_OPTION_HOST_NAME                  12
#define DHCP_OPTION_BOOT_SIZE                  13
#define DHCP_OPTION_MERIT_DUMP_FILE            14
#define DHCP_OPTION_DOMAIN_NAME                15
#define DHCP_OPTION_SWAP_SERVER                16
#define DHCP_OPTION_ROOT_PATH                  17
#define DHCP_OPTION_EXTENSIONS_PATH            18

/* IP Layer Parameters per Host */
#define DHCP_OPTION_IP_FORWARDING              19
#define DHCP_OPTION_NON_LOCAL_SOURCE_ROUTING   20
#define DHCP_OPTION_POLICY_FILTER              21
#define DHCP_OPTION_MAX_DGRAM_REASSEMBLY       22
#define DHCP_OPTION_DEFAULT_IP_TTL             23
#define DHCP_OPTION_PATH_MTU_AGING_TIMEOUT     24
#define DHCP_OPTION_PATH_MTU_PLATEAU_TABLE     25

/* IP Layer Parameters per Interface */
#define DHCP_OPTION_INTERFACE_MTU              26
#define DHCP_OPTION_ALL_SUBNETS_LOCAL          27
#define DHCP_OPTION_BROADCAST_ADDRESS          28
#define DHCP_OPTION_PERFORM_MASK_DISCOVERY     29
#define DHCP_OPTION_MASK_SUPPLIER              30
#define DHCP_OPTION_ROUTER_DISCOVERY           31
#define DHCP_OPTION_ROUTER_SOLICITATION_ADDR   32
#define DHCP_OPTION_STATIC_ROUTE               33

/* Link Layer Parameters per Interface */
#define DHCP_OPTION_TRAILER_ENCAP              34
#define DHCP_OPTION_ARP_CACHE_TIMEOUT          35
#define DHCP_OPTION_ETHERNET_ENCAP             36

/* TCP Parameters */
#define DHCP_OPTION_TCP_DEFAULT_TTL            37
#define DHCP_OPTION_TCP_KEEPALIVE_INTERVAL     38
#define DHCP_OPTION_TCP_KEEPALIVE_GARBAGE      39

/* Application and Service Parameters */
#define DHCP_OPTION_NIS_DOMAIN                 40
#define DHCP_OPTION_NIS_SERVERS                41
#define DHCP_OPTION_NTP_SERVERS                42
#define DHCP_OPTION_VENDOR_SPECIFIC_INFO       43
#define DHCP_OPTION_NETBIOS_NAME_SERVER        44
#define DHCP_OPTION_NETBIOS_DGRAM_DIST_SERVER  45
#define DHCP_OPTION_NETBIOS_NODE_TYPE          46
#define DHCP_OPTION_NETBIOS_SCOPE              47
#define DHCP_OPTION_X_WINDOW_SYSTEM_FONT       48
#define DHCP_OPTION_X_WINDOW_SYSTEM_DISPLAY    49
#define DHCP_OPTION_REQUESTED_IP_ADDR          50
#define DHCP_OPTION_IP_ADDRESS_LEASE_TIME      51
#define DHCP_OPTION_OVERLOAD                   52
#define DHCP_OPTION_TFTP_SERVER_NAME           66
#define DHCP_OPTION_BOOTFILE_NAME              67
#define DHCP_OPTION_DHCP_MESSAGE_TYPE          53
#define DHCP_OPTION_SERVER_IDENTIFIER          54
#define DHCP_OPTION_PARAMETER_REQUEST_LIST     55
#define DHCP_OPTION_MESSAGE                    56
#define DHCP_OPTION_MAX_DHCP_MESSAGE_SIZE      57
#define DHCP_OPTION_RENEWAL_T1_TIME_VALUE      58
#define DHCP_OPTION_REBINDING_T2_TIME_VALUE    59
#define DHCP_OPTION_VENDOR_CLASS_ID            60
#define DHCP_OPTION_CLIENT_ID                  61

#define DHCP_OPTION_TFTP_SERVER_IP             150
#define DHCP_OPTION_ETHERBOOT                  175
#define DHCP_OPTION_IP_TELEPHONE               176

#define DHCP_OPTION_END                        255

__BEGIN_DECLS

// Function to parse DHCP options
// Implementation would be needed in the corresponding .c file
// void parse_dhcp_options(const uint8_t *options, int length);

__END_DECLS

#endif // __DHCP_H__