#include <event/states.h>

#include <stdlib.h>
#include <stdio.h>

pthread_key_t thread_state_key;

static void free_state(void* local) {
    LocalState *state = local;

    fflush(state->log_file);

    char last_msg[64];
    int len = sprintf(last_msg, "Thread %s with pid %d is going to end\n", state->my_name, state->my_pid);
    write(STDOUT_FILENO, last_msg, len);
    
    // Some names are static, don't free them
    // free((void*)state->my_name);

    // TODO: why it causes double free ?
    // free(state);
    
    // Don't close file because there will be race condition if multiple threads use the same log file, let the OS close it
    // fclose(state->log_file);
}

void create_thread_state_key() {
    assert(pthread_key_create(&thread_state_key, free_state) == 0);
}

void set_local_state(LocalState* new_state) {
    LocalState* old_state = pthread_getspecific(thread_state_key);
    if (old_state == NULL) {
        pthread_setspecific(thread_state_key, new_state);
    } else {
        LOG_ERR("There is already a local state, can't call set_local_state() again !");
        free(new_state);
    }
}

LocalState* get_local_state() {
    LocalState *local = pthread_getspecific(thread_state_key);
    if (local) {
        return local;
    } else {
        return NULL;
    }
}
