#include "unity.h"
#include <netutil/checksum.h>
#include <netutil/ip.h>

void test_tcp_checksum_in_net_order_ipv4(void) {
    uint8_t data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, 0);
    uint16_t checksum = tcp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xC6CC; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, sizeof(data));
    checksum = tcp_checksum_in_net_order(data, pheader);
    expected_checksum = 0xEC7A; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_tcp_checksum_in_net_order_zero_ipv4(void) {
    TEST_IGNORE_MESSAGE ("Should test the case that the data is not a multiple of 16 bits");
    uint8_t data[] = "\0x45\0x00\0x00\0x28\0x00\0x00\0x40\0x00\0x40\0x06\0xb8\0x61\0xc0\0xa8\0x00\0x01\0xc0\0xa8\0x00\0xc7";
    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_TCP, sizeof(data));
    uint16_t checksum = tcp_checksum_in_net_order(data, pheader);
    uint16_t expected_checksum = 0x0000; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    TEST_IGNORE_MESSAGE ("Should test the case that the checksum is 0xFFFF");
    TEST_IGNORE_MESSAGE ("Should test the case that the checksum is 0x0000");
}

void test_tcp_checksum_in_net_order_ipv6(void) {
    uint8_t data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_TCP, 0);
    uint16_t checksum = tcp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xF9FF; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_TCP, sizeof(data));
    uint16_t checksum = tcp_checksum_in_net_order(data, pheader);
    uint16_t expected_checksum = 0x1FAE; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_udp_checksum_in_net_order_ipv4(void) {
    uint8_t data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_UDP, 0);
    uint16_t checksum = udp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xBBCC; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv4(0x12345678, 0x87654321, IP_PROTO_UDP, sizeof(data));
    checksum = udp_checksum_in_net_order(data, pheader);
    expected_checksum = 0xE17A; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_udp_checksum_in_net_order_ipv6(void) {
    uint8_t data[128] = "Hello World";

    struct pseudo_ip_header_in_net_order pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_UDP, 0);
    uint16_t checksum = udp_checksum_in_net_order(NULL, pheader);
    uint16_t expected_checksum = 0xEEFF; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    pheader = PSEUDO_HEADER_IPv6(0x12345678, 0x87654321, IPv6_PROTO_UDP, sizeof(data));
    checksum = udp_checksum_in_net_order(data, pheader);
    expected_checksum = 0x14AE; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void test_inet_checksum(void) {
    uint8_t data[256] = "Hello World";
    uint16_t checksum = inet_checksum(data, sizeof(data));
    uint16_t expected_checksum = 0x1FAE; 
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    // Empty string
    checksum = inet_checksum(NULL, 0);
    expected_checksum = 0xFFFF;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    strncpy(data, "Another String", sizeof(data));
    checksum = inet_checksum(data, sizeof(data));
    expected_checksum = 0x1FAD;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);

    // One byte
    checksum = inet_checksum(data, 1);
    expected_checksum = 0x0041;
    TEST_ASSERT_EQUAL_UINT16(expected_checksum, checksum);
}

void all_checksum_tests(void) {
    test_tcp_checksum_in_net_order_ipv4();
    test_tcp_checksum_in_net_order_zero_ipv4();
    test_tcp_checksum_in_net_order_ipv6();

    test_udp_checksum_in_net_order_ipv4();
    test_udp_checksum_in_net_order_ipv6();

    test_inet_checksum();
}
