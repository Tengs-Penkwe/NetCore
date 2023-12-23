#ifndef __EVENT_TIMER_H__
#define __EVENT_TIMER_H__

#include <common.h>
#include "threadpool.h"

__BEGIN_DECLS

#define TIMER_SIG SIGUSR1

typedef uint64_t delayed_us;

errval_t timer_init(void);

void submit_delayed_task(delayed_us delay, task_t task);

__END_DECLS

#endif // __EVENT_TIMER_H__