#include "unity.h"
#include <netutil/etharp.h>

void test_maccmp_identical(void) {
    mac_addr mac1 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    mac_addr mac2 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    TEST_ASSERT_TRUE(maccmp(mac1, mac2));
}

void test_maccmp_different(void) {
    mac_addr mac1 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    mac_addr mac2 = {.addr = {0xAB, 0x89, 0x67, 0x45, 0x23, 0x01}};
    TEST_ASSERT_FALSE(maccmp(mac1, mac2));
}

void test_maccmp_one_zero(void) {
    mac_addr mac1 = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    mac_addr mac2 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    TEST_ASSERT_FALSE(maccmp(mac1, mac2));
}

void test_maccmp_both_zero(void) {
    mac_addr mac1 = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    mac_addr mac2 = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    TEST_ASSERT_TRUE(maccmp(mac1, mac2));
}

void test_maccmp_one_byte_diff(void) {
    mac_addr mac1 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    mac_addr mac2 = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAC}};
    TEST_ASSERT_FALSE(maccmp(mac1, mac2));
}

void all_maccmp_tests(void) {
    test_maccmp_identical();
    test_maccmp_different();
    test_maccmp_one_zero();
    test_maccmp_both_zero();
    test_maccmp_one_byte_diff();
}

void test_tomac_typical(void) {
    uint64_t input = 0x0123456789AB;
    mac_addr expected = {.addr = {0xAB, 0x89, 0x67, 0x45, 0x23, 0x01}};
    mac_addr result = u64tomac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_tomac_zero(void) {
    uint64_t input = 0x0;
    mac_addr expected = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    mac_addr result = u64tomac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_tomac_max(void) {
    uint64_t input = 0xFFFFFFFFFFFF;
    mac_addr expected = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    mac_addr result = u64tomac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_tomac_lower_byte(void) {
    uint64_t input = 0x01;
    mac_addr expected = {.addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
    mac_addr result = u64tomac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_tomac_upper_byte(void) {
    uint64_t input = 0x010000000000;
    mac_addr expected = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
    mac_addr result = u64tomac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void all_tomac_tests(void) {
    test_tomac_typical();
    test_tomac_zero();
    test_tomac_max();
    test_tomac_lower_byte();
    test_tomac_upper_byte();
}

void test_frommac_typical(void) {
    mac_addr input = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    uint64_t expected = 0xAB8967452301;
    uint64_t result = mactou64(input);
    TEST_ASSERT_EQUAL_UINT64(expected, result);
}

void test_frommac_zero(void) {
    mac_addr input = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    uint64_t expected = 0x0;
    uint64_t result = mactou64(input);
    TEST_ASSERT_EQUAL_UINT64(expected, result);
}

void test_frommac_max(void) {
    mac_addr input = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    uint64_t expected = 0xFFFFFFFFFFFF;
    uint64_t result = mactou64(input);
    TEST_ASSERT_EQUAL_UINT64(expected, result);
}

void test_frommac_lower_byte(void) {
    mac_addr input = {.addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
    uint64_t expected = 0x01;
    uint64_t result = mactou64(input);
    TEST_ASSERT_EQUAL_UINT64(expected, result);
}

void test_frommac_upper_byte(void) {
    mac_addr input = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
    uint64_t expected = 0x010000000000;
    uint64_t result = mactou64(input);
    TEST_ASSERT_EQUAL_UINT64(expected, result);
}

void all_frommac_tests(void) {
    test_frommac_typical();
    test_frommac_zero();
    test_frommac_max();
    test_frommac_lower_byte();
    test_frommac_upper_byte();
}

void test_mem2mac_typical(void) {
    uint8_t input[6] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
    mac_addr expected = {.addr = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    mac_addr result = mem2mac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_mem2mac_zero(void) {
    uint8_t input[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    mac_addr expected = {.addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    mac_addr result = mem2mac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void test_mem2mac_max(void) {
    uint8_t input[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    mac_addr expected = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    mac_addr result = mem2mac(input);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, ETH_ADDR_LEN);
}

void all_mem2mac_tests(void) {
    test_mem2mac_typical();
    test_mem2mac_zero();
    test_mem2mac_max();
}

void test_voidptr2mac_normal(void) {
    uint64_t ptr_value = 0x010203040506;
    mac_addr expected = u64tomac(ptr_value);
    void* ptr = (void*)ptr_value;
    mac_addr result = voidptr2mac(ptr);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.addr, result.addr, 6);
}

void test_mac_is_ndp_true(void) {
    mac_addr mac = {.addr = {0x33, 0x33, 0x01, 0x02, 0x03, 0x04}};
    TEST_ASSERT_TRUE(mac_is_ndp(mac));
}

void test_mac_is_ndp_false(void) {
    mac_addr mac = {.addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    TEST_ASSERT_FALSE(mac_is_ndp(mac));
}

void test_get_mac_type_broadcast(void) {
    mac_addr mac = MAC_BROADCAST;
    TEST_ASSERT_EQUAL(MAC_TYPE_BROADCAST, get_mac_type(mac));
}

void test_get_mac_type_multicast(void) {
    mac_addr mac = {.addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    TEST_ASSERT_EQUAL(MAC_TYPE_MULTICAST, get_mac_type(mac));

    mac_addr mac1 = { .addr = { 0x03, 0x02, 0x03, 0x04, 0x05, 0x06 } };
    TEST_ASSERT_EQUAL(MAC_TYPE_MULTICAST, get_mac_type(mac1));
    
    mac_addr mac2 = {.addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
    TEST_ASSERT_EQUAL(MAC_TYPE_MULTICAST, get_mac_type(mac2));
}

void test_get_mac_type_unicast(void) {
    mac_addr mac = {.addr = {0x00, 0x02, 0x03, 0x04, 0x05, 0x06}};
    TEST_ASSERT_EQUAL(MAC_TYPE_UNICAST, get_mac_type(mac));

    mac_addr mac1 = {.addr = {0x02, 0x02, 0x03, 0x04, 0x05, 0x06}};
    TEST_ASSERT_EQUAL(MAC_TYPE_UNICAST, get_mac_type(mac1));
    
    mac_addr mac2 = {.addr = {0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    TEST_ASSERT_EQUAL(MAC_TYPE_UNICAST, get_mac_type(mac2));

    mac_addr mac3 = {.addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00}};
    TEST_ASSERT_EQUAL(MAC_TYPE_UNICAST, get_mac_type(mac3));
    
    uint64_t mac4 = 0xA76ADC05DDE2; // e2:dd:05:dc:6a:a7 in little endian
    TEST_ASSERT_EQUAL(MAC_TYPE_UNICAST, get_mac_type(u64tomac(mac4)));
}

void test_get_mac_type_null(void) {
    mac_addr mac = MAC_NULL;
    TEST_ASSERT_EQUAL(MAC_TYPE_NULL, get_mac_type(mac));
}

void all_ether_tests(void) {
    all_maccmp_tests();
    all_tomac_tests();
    all_frommac_tests();
    all_mem2mac_tests();

    test_voidptr2mac_normal();
    
    test_mac_is_ndp_false();
    test_mac_is_ndp_true();

    test_get_mac_type_broadcast();
    test_get_mac_type_multicast();
    test_get_mac_type_unicast();
    test_get_mac_type_null();
}
