#ifndef __NETSTACK_TYPE_H__
#define __NETSTACK_TYPE_H__

#include <common.h>
#include <lock_free/memorypool.h>

typedef struct buffer {
    uint8_t       *data;      // Not the real start
    uint16_t       from_hdr;  // How many bytes before the data
                              
    uint16_t       valid_size;
    uint16_t       whole_size;

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
        buf.data, buf.from_hdr, buf.valid_size, buf.whole_size, buf.from_pool, buf.mempool);
}

static inline void free_buffer(Buffer buf) {
    if (buf.from_pool) {
        assert(buf.mempool);
        pool_free(buf.mempool, buf.data - buf.from_hdr);
    } else {
        assert(buf.mempool == NULL);
        free(buf.data - buf.from_hdr);
    }
}

static inline Buffer buffer_add(Buffer buf, uint16_t size) {
    assert(buf.from_hdr + size <= buf.whole_size);
    buf.data       += size;
    buf.from_hdr   += size;
    buf.valid_size -= size;
    return buf;
}

static inline void buffer_add_ptr(Buffer *buf, uint16_t size) {
    assert(buf != NULL);
    assert(buf->from_hdr + size <= buf->whole_size);
    buf->data       += size;
    buf->from_hdr   += size;
    buf->valid_size -= size;
}

static inline Buffer buffer_sub(Buffer buf, uint16_t size) {
    assert(buf.from_hdr - size >= 0);
    buf.data       -= size;
    buf.from_hdr   -= size;
    buf.valid_size += size;
    return buf;
}

static inline void buffer_sub_ptr(Buffer *buf, uint16_t size) {
    assert(buf);
    assert(buf->from_hdr - size >= 0);
    buf->data       -= size;
    buf->from_hdr   -= size;
    buf->valid_size += size;
}

__END_DECLS

#endif // __NETSTACK_TYPE_H__