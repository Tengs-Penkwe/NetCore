#include "unity.h"
#include <netstack/icmp.h>
#include <netutil/icmp.h>

void test_IcmpInit(void) {
    TEST_IGNORE_MESSAGE ("Not implemented yet");
    ICMP icmp;
    struct ip_state ip;

    errval_t result = icmp_init(&icmp, &ip);
    TEST_ASSERT_EQUAL(SYS_ERR_OK, result);
}

void test_IcmpMarshalUnmarshal(void) {
    TEST_IGNORE_MESSAGE ("Not implemented yet");
    ICMP icmp;
    struct ip_state ip;
    icmp_init(&icmp, &ip);

    ip_addr_t dst_ip = 0xC0A80001; // 192.168.0.1
    uint8_t type = ICMP_ECHO;
    uint8_t code = 0;
    ICMP_data field;
    Buffer buf;
    // Initialize buffer and field appropriately

    errval_t marshal_result = icmp_marshal(&icmp, dst_ip, type, code, field, buf);
    TEST_ASSERT_EQUAL(SYS_ERR_OK, marshal_result);

    errval_t unmarshal_result = icmp_unmarshal(&icmp, dst_ip, buf);
    TEST_ASSERT_EQUAL(SYS_ERR_OK, unmarshal_result);
}
