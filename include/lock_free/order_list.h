#ifndef __LOCK_FREE_LIST_H__
#define __LOCK_FREE_LIST_H__

#include <common.h>      // BEGIN, END DECLS
#include "defs.h"
#include "liblfds711.h"  // Lock-free structures

enum ordlist_policy {
    LS_OVERWRITE_ON_EXIST,
    LS_FAIL_ON_EXIST,
};

typedef struct {
    alignas(ATOMIC_ISOLATION) 
        struct lfds711_list_aso_state list;
    enum ordlist_policy                  policy;
} OrdList __attribute__((aligned(ATOMIC_ISOLATION)));

__BEGIN_DECLS

errval_t ordlist_init(OrdList* list, list_key_compare cmp_func, enum ordlist_policy policy);
void ordlist_destroy(OrdList* list);
errval_t ordlist_insert(OrdList* list, void* data);
errval_t ordlist_get_by_key(OrdList* list, void* key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_LIST_H__