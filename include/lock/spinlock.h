#ifndef __LOCK_SPINLOCK_H__
#define __LOCK_SPINLOCK_H__

#include <stdatomic.h>

static inline void spin_lock(atomic_flag* lock) {
    if (atomic_flag_test_and_set(lock)) {
        // Busy Wait
    }
}

static inline void spin_unlock(atomic_flag* lock) {
    atomic_flag_clear(lock);
}

#endif // __LOCK_SPINLOCK_H__