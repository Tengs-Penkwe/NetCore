#ifndef __LOCK_FREE_H__
#define __LOCK_FREE_H__

#include <common.h>      // BEGIN, END DECLS
#include "liblfds711.h"  // Lock-free structures

#define CORES_SYNC_BARRIER  LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE
#define ATOMIC_ISOLATION    LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES

__BEGIN_DECLS

typedef int (*list_key_compare)(void const *new_key, void const *existing_key);

__attribute__((unused))
static void umm_queue_element_cleanup_callback(struct lfds711_queue_umm_state *qs, struct lfds711_queue_umm_element *qe, enum lfds711_misc_flag dummy_flag) {
    if (dummy_flag != LFDS711_MISC_FLAG_RAISED) {
        assert(qs && qe);
        free(qe);
    }
}

__attribute__((unused))
static void hash_element_cleanup_callback(struct lfds711_hash_a_state *has, struct lfds711_hash_a_element *hae) {
    assert(has && hae);
    // free(hae);
    // Since we use the pointer inside the element as data, we don't need to free the data.
}

__attribute__((unused))
static void freelist_element_cleanup_callback(struct lfds711_freelist_state *fs, struct lfds711_freelist_element *fe) {
    assert(fs && fe);
    struct lfds711_hash_a_element* he = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*fe);
    assert(he);
    free(he);
}

__END_DECLS

#endif // __LOCK_FREE_H__
