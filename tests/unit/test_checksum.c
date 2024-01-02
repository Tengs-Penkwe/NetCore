#include "unity.h"
#include <netutil/checksum.h>
#include <netutil/ip.h>

void test_tcp_checksum_in_net_order_ipv4(void) {
    char data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, 0);
    uint16_t checksum = tcp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = ~ntohs(0x3339); 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, sizeof(data));
    checksum = tcp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x787A; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, 17);
    checksum = tcp_checksum_in_net_order(data, pheader);
    expected_checksum = 0xE77A; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_tcp_checksum_in_net_order_ipv6(void) {
    char data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_TCP, 0);
    uint16_t checksum = tcp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = ~ntohs(0x3339); 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_TCP, sizeof(data));
    checksum = tcp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x787A;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    memcpy(&pheader, 
    "\xFE\x80\x00\x00\x00\x00\x00\x00\xE0\xDD\x05\xFF\xFE\xDC\x6A\xA7"  // src ip: fe80::e0dd:5ff:fedc:6aa7
    "\xFF\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x16"  // dst ip: ff02::16
    "\x00\x00\x00\x1C"      // length = 28
    "\x00\x00\x00\x3A"      // next header = 58 (ICMPv6)
    , 40);
    memcpy(data,
    "\x8F\x00\xB5\x24"      // ICMPv6 type = 143, code = 0, checksum = 0xB524
    "\x00\x00\x00\x01\x04\x00\x00\x00"
    "\xFF\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xFF\xDC\x6A\xA7"  // ipv6 ff02::1:ffdc:6aa7
    , 28);
    checksum = tcp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x0000; // 0xB524
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_udp_checksum_in_net_order_ipv4(void) {
    char data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_UDP, 0);
    uint16_t checksum = udp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xBBCC; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_UDP, sizeof(data));
    checksum = udp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x6D7A; //0xE17A; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_udp_checksum_in_net_order_ipv6(void) {
    char data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_UDP, 0);
    uint16_t checksum = udp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xBBCC; //0xEEFF; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_UDP, sizeof(data));
    checksum = udp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x6D7A;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_inet_checksum(void) { 
    char data[128] = "Hello World";
    uint16_t checksum = inet_checksum_in_net_order(data, sizeof(data));
    uint16_t expected_checksum = 0x31AE;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    // Empty string
    checksum = inet_checksum_in_net_order(NULL, 0);
    expected_checksum = 0xFFFF;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    strncpy(data, "Another String", sizeof(data));
    checksum = inet_checksum_in_net_order(data, sizeof(data));
    expected_checksum = 0x5240;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    // One byte
    checksum = inet_checksum_in_net_order(data, 1);
    expected_checksum = 0xFFBE;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void all_checksum_tests(void) {
    test_tcp_checksum_in_net_order_ipv4();
    test_tcp_checksum_in_net_order_ipv6();

    test_udp_checksum_in_net_order_ipv4();
    test_udp_checksum_in_net_order_ipv6();

    test_inet_checksum();

    TEST_IGNORE_MESSAGE ("Should test the case that the checksum is 0xFFFF");
    TEST_IGNORE_MESSAGE ("Should test the case that the checksum is 0x0000");
}
