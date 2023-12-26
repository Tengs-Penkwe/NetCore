#ifndef __LOCK_FREE_LIST_H__
#define __LOCK_FREE_LIST_H__

#include <common.h>      // BEGIN, END DECLS
#include "defs.h"
#include "liblfds711.h"  // Lock-free structures

typedef struct lfds711_list_asu_state List;
typedef struct lfds711_list_asu_element LTele;

__BEGIN_DECLS

errval_t list_init(List* list);
void list_destroy(List* list);
errval_t list_insert_at_start(List* list, void* data);
errval_t list_insert_at_end(List* list, void* data);
errval_t list_insert_at(List* list, void* data);
errval_t list_insert_after(List* list, void* data);
errval_t list_get_by_key(List* list, void* key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_LIST_H__