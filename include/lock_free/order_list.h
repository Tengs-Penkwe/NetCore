#ifndef __LOCK_FREE_LIST_H__
#define __LOCK_FREE_LIST_H__

#include <common.h>      // BEGIN, END DECLS
#include "liblfds711.h"  // Lock-free structures

enum list_policy {
    LS_OVERWRITE_ON_EXIST,
    LS_FAIL_ON_EXIST,
};

typedef struct {
    alignas(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES) 
        struct lfds711_list_aso_state list;
    enum list_policy                  policy;
} OrdList;

typedef int (*list_key_compare)(void const *new_key, void const *existing_key);

static inline void list_init_barrier(void)
{
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
}

__BEGIN_DECLS

errval_t list_init(OrdList* list, list_key_compare cmp_func, enum list_policy policy);
void list_destroy(OrdList* list);
errval_t list_insert(OrdList* list, void* data);
errval_t list_get_by_key(OrdList* list, void* key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_LIST_H__