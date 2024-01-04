#ifndef __NETSTACK_TYPE_H__
#define __NETSTACK_TYPE_H__

#include <common.h>
#include <event/memorypool.h>

typedef struct buffer {
    uint8_t       *data;      // Not the real start
    uint16_t       from_hdr;  // How many bytes before the data
                              
    uint32_t       valid_size;
    uint32_t       whole_size; 

    bool           from_pool;
    MemPool       *mempool;
} Buffer ;

#define NULL_BUFFER        \
    (struct buffer)        \
    {   NULL, 0,           \
        0, 0,              \
        false, NULL        \
    }


__BEGIN_DECLS

static inline void dump_buffer(Buffer buf)
{
    printf("Buffer: data: %p, \tfrom_hdr: %d, \tvalid_size: %d, \twhole_size: %d, \n\tfrom_pool: %d, \tmempool: %p\n",
        (void*)buf.data, buf.from_hdr, buf.valid_size, buf.whole_size, buf.from_pool, (void *)buf.mempool);
}

static inline void free_buffer(Buffer buf) {
    uint8_t* original_data = buf.data - (size_t)buf.from_hdr;
    if (buf.from_pool) {
        assert(buf.mempool);
        pool_free(buf.mempool, original_data);
    } else {
        assert(buf.mempool == NULL);
        free(original_data);
    }
}

static inline Buffer buffer_create(uint8_t *data, uint32_t from_hdr, uint32_t valid_size, uint32_t whole_size, bool from_pool, MemPool *mempool) {
    assert(data);
    assert(from_hdr <= UINT16_MAX && valid_size <= UINT16_MAX && whole_size <= UINT16_MAX);
    assert(from_hdr <= whole_size && valid_size <= whole_size); // Overflow check, although we use uint32_t, we check the uint16_t part
    if (from_pool) assert(mempool); else assert(mempool == NULL);
    return (Buffer) {
        .data       = data,
        .from_hdr   = (uint16_t)from_hdr,
        .valid_size = valid_size,
        .whole_size = whole_size,
        .from_pool  = from_pool,
        .mempool    = mempool,
    };
}

static inline void buffer_add_ptr(Buffer *buf, uint16_t size) {
    assert(buf);
    assert(buf->from_hdr + size <= buf->whole_size);
    assert(buf->valid_size >= size);    // Underflow check
    buf->data       += size;
    buf->from_hdr   += size;
    buf->valid_size -= size;
}

static inline Buffer buffer_add(Buffer buf, uint16_t size) {
    buffer_add_ptr(&buf, size);
    return buf;
}

static inline void buffer_sub_ptr(Buffer *buf, uint16_t size) {
    assert(buf);
    assert(buf->from_hdr - size >= 0);
    assert(UINT16_MAX - buf->valid_size >= size);   // Overflow check
    buf->data       -= size;
    buf->from_hdr   -= size;
    buf->valid_size += size;
}

static inline Buffer buffer_sub(Buffer buf, uint16_t size) {
    buffer_sub_ptr(&buf, size);
    return buf;
}

static inline void buffer_reclaim_ptr(Buffer* buf, uint16_t from_hdr, uint16_t valid_size) {
    assert(buf);
    assert(from_hdr + valid_size <= buf->whole_size);
    buf->data       = buf->data - buf->from_hdr + from_hdr;
    buf->from_hdr   = from_hdr;
    buf->valid_size = valid_size;
}

static inline Buffer buffer_reclaim(Buffer buf, uint16_t from_hdr, uint16_t valid_size) {
    buffer_reclaim_ptr(&buf, from_hdr, valid_size);
    return buf;
}


__END_DECLS

#endif // __NETSTACK_TYPE_H__
