#include <event/signal.h>
#include <sys/resource.h>  //getrlimit
#include <event/timer.h>
#include <errno.h>

errval_t signal_init(rlim_t queue_size)
{
    errval_t err = SYS_ERR_OK;
    // Note: we are in main thread !

    assert(TIMER_NUM <= (SIGRTMAX - SIGRTMIN) && "Timer number must be less than the number of real-time signals");

    // 1. Mask the SIG_TIGGER_SUBMIT (timer thread will unmask it)
    sigset_t set;
    sigemptyset(&set);
    for (int i = 0; i < TIMER_NUM; i++) {
        sigaddset(&set, SIG_TIGGER_SUBMIT + i);
    }
    if (sigprocmask(SIG_BLOCK, &set, NULL) != 0)
    {
        LOG_FATAL("sigprocmask: %s", strerror(errno));
        return EVENT_ERR_SIGNAL_INIT;
    }

    // 2.1 Set the signal queue limits
    struct rlimit rl;
    if (getrlimit(RLIMIT_SIGPENDING, &rl) == -1)
    {
        LOG_FATAL("gettrlimit: %s", strerror(errno));
        return EVENT_ERR_SIGNAL_INIT;
    }
    LOG_NOTE("Current signal queue limits: soft=%ld; hard=%ld, need=%ld", rl.rlim_cur, rl.rlim_max, queue_size);

    if (rl.rlim_max < queue_size) {
        rl.rlim_max = queue_size;
    }

    if (rl.rlim_cur < rl.rlim_max) {
        rl.rlim_cur = rl.rlim_max; // = queue_size;

        // 2.2 Set a new limit
        if (setrlimit(RLIMIT_SIGPENDING, &rl) == -1)
        {
            LOG_FATAL("settrlimit: %s", strerror(errno));
            return EVENT_ERR_SIGNAL_INIT;
        }
        LOG_NOTE("New limits set - soft: %ld, hard: %ld (-1 means unlimited)", rl.rlim_cur, rl.rlim_max);
    }

    return err;
}