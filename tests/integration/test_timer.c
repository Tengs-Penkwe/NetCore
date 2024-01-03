#include "unity.h"
#include <event/timer.h>
#include <event/signal.h>
#include <event/threadpool.h>
#include <event/states.h>
#include <syscall.h>
#include <sys/resource.h>  //getrlimit
#include <unistd.h>
#include <stdatomic.h>

void timer_setup(void) {
    create_thread_state_key();

    g_states.log_file = fopen("log/test_timer_log.json", "w");
    LocalState *master = calloc(1, sizeof(LocalState));
    *master = (LocalState) {
        .my_name   = "Test",
        .my_pid    = syscall(SYS_gettid),
        .log_file  = (g_states.log_file == NULL) ? stdout : g_states.log_file,
        .my_state  = NULL,
    };
    set_local_state(master);

    assert(signal_init(true) == SYS_ERR_OK); 

    // 1. Initialize the thread pool
    assert(thread_pool_init(4) == SYS_ERR_OK);

    // 2. Initialize the timer thread (timed event)
    assert(timer_thread_init(g_states.timer) == SYS_ERR_OK);
}

atomic_size_t g_count = 0;

void simple_task(void* args) {
    (void) args;
    atomic_fetch_add(&g_count, 1);
}

void test_timer(void) {

    timer_setup();

    size_t event_count = 1000000;   // 1 Million

    for (size_t i = 0; i < event_count; i++) {  
        // Random delay between 1ms and 1s
        delayed_us delay = rand() % 1000000 + 1000;

        Task task = {
            .queue   = &g_threadpool.queue,
            .sem     = &g_threadpool.sem,
            .process = simple_task,
            .arg     = NULL,
        };
        submit_delayed_task(MK_DELAY_TASK(delay, NULL, task));
    }
    sleep(3);

    size_t all_submitted = 0;
    size_t all_recvd     = 0;
    size_t all_failed    = 0;

    for (size_t i = 0; i < TIMER_NUM; i++) {
        all_submitted += g_states.timer[i].count_submitted;
        all_recvd     += g_states.timer[i].count_recvd;
        all_failed    += g_states.timer[i].count_failed;
        printf("Timer %d: submitted: %zu, recvd: %zu, failed: %zu\n", (int)i,
               g_states.timer[i].count_submitted, g_states.timer[i].count_recvd,
               g_states.timer[i].count_failed);
    }

    TEST_ASSERT_EQUAL(all_submitted, event_count);
    TEST_ASSERT_EQUAL(all_recvd, event_count);
    TEST_ASSERT_EQUAL(all_failed, 0);
    TEST_ASSERT_EQUAL(g_count, event_count);
}
