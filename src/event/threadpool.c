#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>
#include <errno.h>         //sterror
#include <event/states.h>

#include <sys/syscall.h>   //syscall
#include <sys/types.h>     //pid_t

// Global variable defined in threadpool.h
alignas(ATOMIC_ISOLATION) ThreadPool g_threadpool;
// TODO: move to g_states, we don't want to manage many global variables

errval_t thread_pool_init(size_t workers) 
{
    errval_t err;
    assert(workers > 0);
    g_threadpool.workers = workers;

    // 1. Unbounded, MPMC queue
    err = bdqueue_init(&g_threadpool.queue, g_threadpool.elements, TASK_QUEUE_SIZE);
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock free queue");

    // 1.2 Semaphore to notify woker 
    if (sem_init(&g_threadpool.sem, 0, 0) != 0) {
        const char *error_msg = strerror(errno);
        EVENT_FATAL("Can't initialize the semaphore: %s", error_msg);
        return SYS_ERR_INIT_FAIL;
    }

    // 3. Create all the workers
    g_threadpool.threads  = calloc(workers, sizeof(pthread_t));
    LocalState* local = calloc(workers, sizeof(LocalState));
    
    for (size_t i = 0; i < workers; i++)
    {
        char* name = calloc(16, sizeof(char));
        sprintf(name, "Slave%d", (int)i);

        local[i] = (LocalState) {
            .my_name  = name,
            .my_pid   = (pid_t)-1,      // Don't know yet
            .log_file = (g_states.log_file == NULL) ? stdout : g_states.log_file,
            .my_state = &g_threadpool,
        };

        if (pthread_create(&g_threadpool.threads[i], NULL, thread_function, (void*)&local[i]) != 0) {
            LOG_FATAL("Can't create worker thread");
            free(g_threadpool.threads); free(local);
            return EVENT_ERR_THREAD_CREATE;
        }
    }

    EVENT_NOTE("Thread pool: %d slaves initialized", workers);
    return SYS_ERR_OK;
}

void thread_pool_destroy(void) {
    bdqueue_destroy(&g_threadpool.queue);

    for (size_t i = 0; i < g_threadpool.workers; i++) 
        assert(pthread_cancel(g_threadpool.threads[i]) == 0);
    EVENT_NOTE("TODO: let the thread itself do some cleaning");

    sem_destroy(&g_threadpool.sem);

    free(g_threadpool.threads);
    g_threadpool.threads = NULL;

    EVENT_NOTE(
        "Need to free local states !"
        "Need to Close all opened files !");

    memset(&g_threadpool, 0, sizeof(g_threadpool));
    EVENT_NOTE("Threadpool destroyed !");
}

void *thread_function(void* localstate) {
    assert(localstate);
    LocalState* local = localstate;
    set_local_state(local);
    local->my_pid = syscall(SYS_gettid);
    EVENT_NOTE("ThreadPool %s started with pid %d", local->my_name, local->my_pid);

    // Initialization barrier for lock-free queue
    CORES_SYNC_BARRIER;    
    
    ThreadPool* pool = local->my_state; assert(pool);

    Task *task = NULL;
    while(true) {
        if (debdqueue(&pool->queue, NULL, (void**)&task) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&pool->sem);
        } else {
            assert(task);
            (*task->process)(task->arg);
            free(task);
            task = NULL;
        }
    }
    //TODO: let the threads receive a signal and gracefully exit
}

errval_t submit_task(Task task) {
    errval_t err;

    // free after dequeue
    Task* task_copy = malloc(sizeof(Task));
    *task_copy = task;

    err = enbdqueue(task.queue, NULL, task_copy);
    if (err_no(err) == EVENT_ENQUEUE_FULL) {
        EVENT_WARN("The Task Queue is full !");
        return err;
    } 
    DEBUG_FAIL_RETURN(err, "Error met when trying to enqueue!");

    sem_post(task.sem);
    return SYS_ERR_OK;
}
