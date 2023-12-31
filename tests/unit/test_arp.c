#include "unity.h"
#include <netstack/arp.h>

void setUp(void) {
    // Code to run before each test
}

void tearDown(void) {
    // Code to run after each test
}

void test_ArpInit(void) {
    ARP arp;
    Ethernet ether;
    ip_addr_t ip = 0xC0A80001; // 192.168.0.1 in hex

    errval_t result = arp_init(&arp, &ether, ip);
    TEST_ASSERT_EQUAL(SYS_ERR_OK, result);
}

void test_ArpRegisterAndLookup(void) {
    ARP arp;
    Ethernet ether;
    ip_addr_t ip = 0xC0A80001; // 192.168.0.1
    mac_addr mac = {
        .addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
    };

    arp_init(&arp, &ether, ip);
    arp_register(&arp, ip, mac);

    mac_addr found_mac;
    errval_t result = arp_lookup_mac(&arp, ip, &found_mac);

    TEST_ASSERT_EQUAL(SYS_ERR_OK, result);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mac.addr, found_mac.addr, sizeof(mac_addr));
}

void test_ArpDestroy(void) {
    ARP arp;
    Ethernet ether;
    ip_addr_t ip = 0xC0A80001;

    arp_init(&arp, &ether, ip);
    arp_destroy(&arp);

    // This test might be tricky to implement as it's hard to verify destruction.
    // Depending on your implementation, you might check if resources are freed.
    // For now, just ensuring it doesn't crash might be enough.
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ArpInit);
    RUN_TEST(test_ArpRegisterAndLookup);
    RUN_TEST(test_ArpDestroy);
    return UNITY_END();
}
