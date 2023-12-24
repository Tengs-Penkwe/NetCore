#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define THREAD_POOL_SIZE 8
#define TASK_SIZE        1024

#include <common.h>
#include <pthread.h>
#include <semaphore.h>
#include "liblfds711.h"  // Lock-free structures

typedef struct {
    void (*function)(void*);
    void* argument;
} task_t;

#define MK_TASK(h,a)    (task_t){ /*handler*/ (h), /*arg*/ (a) }
#define NOP_TASK        MK_TASK(NULL, NULL)

#define INIT_QUEUE_SIZE             128
#define ADDITIONAL_LIST_ELEMENTS    4

typedef struct {
    sem_t sem;
    struct lfds711_queue_umm_state   queue;
    struct lfds711_queue_umm_element dummy_element;
    struct lfds711_queue_umm_element elements[INIT_QUEUE_SIZE];

    // struct lfds711_prng_st_state    *psts;
    // struct lfds711_freelist_element
    //      volatile (*elimination_array)[LFDS711_FREELIST_ELIMINATION_ARRAY_ELEMENT_SIZE_IN_FREELIST_ELEMENTS];

    struct lfds711_freelist_state    freelist;
    struct lfds711_freelist_element  list_e[INIT_QUEUE_SIZE];

    pthread_t       *threads;
} Pool;

extern Pool pool;

__BEGIN_DECLS

errval_t thread_pool_init(size_t workers);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
void submit_task(task_t task);
void process_task(task_t task);

__END_DECLS

#endif // __EVENT_THREADPOOL_H__