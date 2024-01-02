#include "unity.h"
#include <netutil/dump.h>

#include <arpa/inet.h>

void test_format_mac_address(void) {
    mac_addr addr = {.addr = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E}};
    char buffer[18]; // MAC addresses are 17 characters long (6 pairs of hex digits and 5 separators)
    char expected[18] = "00:1A:2B:3C:4D:5E";

    format_mac_address(&addr, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    // Additional test cases with different MAC addresses
    addr = (mac_addr){.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA}};
    strcpy(expected, "FF:FF:FF:FF:FF:FA");
    format_mac_address(&addr, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    addr = (mac_addr){.addr = {0x00, 0x02, 0x00, 0x00, 0x00, 0x00}};
    strcpy(expected, "00:02:00:00:00:00");
    format_mac_address(&addr, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    addr = (mac_addr){.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    strcpy(expected, "01:23:45:67:89:AB");
    format_mac_address(&addr, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}

void test_format_ipv4_addr(void) {
    ip_addr_t ip_net = htonl(0xC0A80101); // 192.168.1.1 in network byte order
    char buffer[INET_ADDRSTRLEN];
    char expected[INET_ADDRSTRLEN];

    format_ipv4_addr(ntohl(ip_net), buffer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_net, expected, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    ip_net = htonl(0x00000000); // 0.0.0.0 in network byte order
    format_ipv4_addr(ntohl(ip_net), buffer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_net, expected, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    ip_net = htonl(0xFFFFFFFF); // 255.255.255.255 in network byte order
    format_ipv4_addr(ntohl(ip_net), buffer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_net, expected, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    ip_net = htonl(0x0A00020F); // 10.0.2.15 in network byte order
    format_ipv4_addr(ntohl(ip_net), buffer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_net, expected, INET_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}

void test_format_ipv6_addr(void) {
    char buffer[IPv6_ADDRESTRLEN];
    char expected[INET6_ADDRSTRLEN];

    TEST_ASSERT_EQUAL(46, IPv6_ADDRESTRLEN);
    TEST_ASSERT_EQUAL(46, INET6_ADDRSTRLEN);

    ipv6_addr_t addr = mk_ipv6(0x20010DB800000000, 0x0000000000000001); // 2001:0db8:0000:0000:0000:0000:0000:0001
    format_ipv6_addr(addr, buffer, INET6_ADDRSTRLEN);
    strncpy(expected, "2001:db8:0:0:0:0:0:1", INET6_ADDRSTRLEN); 
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    addr = mk_ipv6(0xFE80000000000000, 0xE0DDA4FFFE00A861); // FE80:0000:0000:0000:E0DD:A4FF:FE00:A861
    format_ipv6_addr(addr, buffer, INET6_ADDRSTRLEN);
    strncpy(expected, "fe80:0:0:0:e0dd:a4ff:fe00:a861", INET6_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    addr = mk_ipv6(0xFE80000000000001, 0xC7C8D9EAFEB0C0D0); // FE80:0000:0000:0000:C7C8:D9EA:FEB0:C0D0
    format_ipv6_addr(addr, buffer, INET6_ADDRSTRLEN);
    strncpy(expected, "fe80:0:0:1:c7c8:d9ea:feb0:c0d0", INET6_ADDRSTRLEN);
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}

void all_format_ip_addr_tests(void) {
    test_format_mac_address();
    test_format_ipv4_addr();
    test_format_ipv6_addr();
}