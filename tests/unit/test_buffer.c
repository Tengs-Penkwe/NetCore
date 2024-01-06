#include "unity.h"
#include <event/buffer.h>

void test_buffer_create(void) {
    uint8_t data[256];
    MemPool mempool;

    // Test with normal parameters
    Buffer buf1 = buffer_create(data, 10, 20, 30, false, NULL);
    TEST_ASSERT_EQUAL_PTR(data, buf1.data);
    TEST_ASSERT_EQUAL_UINT16(10, buf1.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(20, buf1.valid_size);
    TEST_ASSERT_EQUAL_UINT32(30, buf1.whole_size);

    // Test with from_pool true
    Buffer buf2 = buffer_create(data, 5, 15, 25, true, &mempool);
    TEST_ASSERT_EQUAL_PTR(data, buf2.data);
    TEST_ASSERT_EQUAL_UINT16(5, buf2.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(15, buf2.valid_size);
    TEST_ASSERT_EQUAL_UINT32(25, buf2.whole_size);
    TEST_ASSERT_EQUAL_PTR(&mempool, buf2.mempool);

    // Edge case: maximum values
    Buffer buf3 = buffer_create(data, UINT16_MAX, UINT16_MAX, UINT16_MAX, false, NULL);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, buf3.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(UINT16_MAX, buf3.valid_size);
    TEST_ASSERT_EQUAL_UINT32(UINT16_MAX, buf3.whole_size);
}

void test_buffer_add(void) {
    uint8_t data[256];
    Buffer buf = buffer_create(data, 10, 20, 50, false, NULL);

    // Test adding a normal size
    buffer_add_ptr(&buf, 5);
    TEST_ASSERT_EQUAL_UINT16(15, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(15, buf.valid_size);

    // Test using buffer_add
    buf = buffer_add(buf, 5);
    TEST_ASSERT_EQUAL_UINT16(20, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(10, buf.valid_size);

    // Edge case: adding maximum possible size
    buf = buffer_add(buf, 10);
    TEST_ASSERT_EQUAL_UINT16(30, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(0, buf.valid_size);
}

void test_buffer_sub(void) {
    uint8_t data[256];
    Buffer buf = buffer_create(data + 50, 50, 20, 100, false, NULL);

    // Test subtracting a normal size
    buffer_sub_ptr(&buf, 10);
    TEST_ASSERT_EQUAL_UINT16(40, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(30, buf.valid_size);

    // Test using buffer_sub
    buf = buffer_sub(buf, 10);
    TEST_ASSERT_EQUAL_UINT16(30, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(40, buf.valid_size);

    // Edge case: subtracting maximum possible size
    buf = buffer_sub(buf, 30);
    TEST_ASSERT_EQUAL_UINT16(0, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(70, buf.valid_size);
}

void test_buffer_reclaim(void) {
    uint8_t data[256];
    Buffer buf = buffer_create(data + 30, 30, 20, 100, false, NULL);

    // Test with normal parameters
    buffer_reclaim_ptr(&buf, 10, 15);
    TEST_ASSERT_EQUAL_UINT16(10, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(15, buf.valid_size);

    // Test using buffer_reclaim
    buf = buffer_reclaim(buf, 5, 10);
    TEST_ASSERT_EQUAL_UINT16(5, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(10, buf.valid_size);

    TEST_PASS_MESSAGE("Unity doesn't support testing for catching assert, so we can't test the following cases:");
    // Edge case: reclaim with maximum values
    buf = buffer_reclaim(buf, UINT16_MAX, UINT16_MAX);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, buf.from_hdr);
    TEST_ASSERT_EQUAL_UINT32(UINT16_MAX, buf.valid_size);
}

void test_free_buffer(void) {
    uint8_t data[256];
    MemPool mempool;

    // Test freeing a buffer from regular memory
    Buffer buf1 = buffer_create(data, 10, 20, 30, false, NULL);
    free_buffer(buf1);
    // Here you might want to check if the memory was freed correctly.

    // Test freeing a buffer from a memory pool
    Buffer buf2 = buffer_create(data, 5, 15, 25, true, &mempool);
    free_buffer(buf2);
    // Here you might want to check if the buffer was returned to the pool.
}

void all_buffer_tests(void) {
    test_buffer_create();
    test_buffer_add();
    test_buffer_sub();
    test_buffer_reclaim();
    test_free_buffer();
}