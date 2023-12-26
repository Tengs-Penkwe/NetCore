#ifndef __LOCK_FREE_LIST_H__
#define __LOCK_FREE_LIST_H__

#include <common.h>      // BEGIN, END DECLS
#include "defs.h"
#include "liblfds711.h"  // Lock-free structures

enum list_policy {
    LS_OVERWRITE_ON_EXIST,
    LS_FAIL_ON_EXIST,
};

typedef struct {
    alignas(ATOMIC_ISOLATION) 
        struct lfds711_list_aso_state list;
    enum list_policy                  policy;
} OrdList __attribute__((aligned(ATOMIC_ISOLATION)));

typedef int (*list_key_compare)(void const *new_key, void const *existing_key);

__BEGIN_DECLS

errval_t list_init(OrdList* list, list_key_compare cmp_func, enum list_policy policy);
void list_destroy(OrdList* list);
errval_t list_insert(OrdList* list, void* data);
errval_t list_get_by_key(OrdList* list, void* key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_LIST_H__