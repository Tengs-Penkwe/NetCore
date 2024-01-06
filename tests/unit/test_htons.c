#include "unity.h"
#include <netutil/htons.h>

void test_hton6_sequential(void) {
    struct eth_addr input = {.addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    struct eth_addr expected = {.addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    struct eth_addr result = hton6(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(struct eth_addr));
}

void test_hton6_all_same(void) {
    struct eth_addr input = {.addr = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
    struct eth_addr expected = {.addr = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
    struct eth_addr result = hton6(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(struct eth_addr));
}

void test_hton6_max_values(void) {
    struct eth_addr input = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    struct eth_addr expected = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    struct eth_addr result = hton6(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(struct eth_addr));
}

void test_hton6_min_values(void) {
    struct eth_addr input = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    struct eth_addr expected = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    struct eth_addr result = hton6(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(struct eth_addr));
}

void test_hton6_mixed_values(void) {
    struct eth_addr input = {.addr = {0x01, 0xFF, 0x00, 0xAB, 0xCD, 0x23}};
    struct eth_addr expected = {.addr = {0x01, 0xFF, 0x00, 0xAB, 0xCD, 0x23}};
    struct eth_addr result = hton6(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(struct eth_addr));
}

void all_hton6_tests(void) {
    test_hton6_sequential();
    test_hton6_all_same();
    test_hton6_max_values();
    test_hton6_min_values();
    test_hton6_mixed_values();
}

void test_hton16_sequential(void) {
    ipv6_addr_t input    = mk_ipv6(0x0123456789ABCDEF, 0x0123456789ABCDEF);
    ipv6_addr_t result   = hton16(input);
    ipv6_addr_t expected = mk_ipv6(0xEFCDAB8967452301, 0xEFCDAB8967452301);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(ipv6_addr_t));
}

void test_hton16_random(void) {
    ipv6_addr_t input    = mk_ipv6(0x02443ff367c9a3d7, 0x0a0b0c0d0e0f1011);
    ipv6_addr_t result   = hton16(input);
    ipv6_addr_t expected = mk_ipv6(0x11100f0e0d0c0b0a, 0xd7a3c967f33f4402);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(ipv6_addr_t));
}

void test_hton16_alternating(void) {
    ipv6_addr_t input    = mk_ipv6(0xAAAAAAAAAAAAAAAA, 0x5555555555555555);
    ipv6_addr_t expected = mk_ipv6(0x5555555555555555, 0xAAAAAAAAAAAAAAAA);
    ipv6_addr_t result = hton16(input);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &result, sizeof(ipv6_addr_t));
}

void test_hton16_zeros(void) {
    ipv6_addr_t input    = mk_ipv6(0x0000000000000000, 0x0000000000000000);
    ipv6_addr_t expected = mk_ipv6(0x0000000000000000, 0x0000000000000000);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &input, sizeof(ipv6_addr_t));
}

void test_hton16_max_values(void) {
    ipv6_addr_t input    = mk_ipv6(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    ipv6_addr_t expected = mk_ipv6(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &input, sizeof(ipv6_addr_t));
}

void all_hton16_tests(void) {
    test_hton16_sequential();
    test_hton16_alternating();
    test_hton16_random();
    test_hton16_zeros();
    test_hton16_max_values();
}

