#include "unity.h"

void setUp(void) {
    // Code to run before each test
}

void tearDown(void) {
    // Code to run after each test
}

extern void test_ArpInit(void);
extern void test_ArpRegisterAndLookup(void);
extern void test_ArpDestroy(void);

extern void test_IcmpInit(void);
extern void test_IcmpMarshalUnmarshal(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ArpInit);
    RUN_TEST(test_ArpRegisterAndLookup);
    RUN_TEST(test_ArpDestroy);

    RUN_TEST(test_IcmpInit);
    RUN_TEST(test_IcmpMarshalUnmarshal);
    return UNITY_END();
}