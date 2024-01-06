#ifndef __EVENT_SIGNAL_H__
#define __EVENT_SIGNAL_H__

#include <common.h>
#include <sys/resource.h>

errval_t signal_init(rlim_t queue_size);

#endif // __EVENT_SIGNAL_H__