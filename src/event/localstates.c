#include <event/states.h>

#include <stdlib.h>
#include <stdio.h>

pthread_key_t thread_state_key;

static void free_state(void* local) {
    LocalState *state = local;

    fflush(state->log_file);

    char last_msg[64];
    int len =sprintf(last_msg, "Thread %s with pid %d is going to end\n", state->my_name, state->my_pid);
    write(STDOUT_FILENO, last_msg, len);

    // Don't close file because there will be race condition, let the OS close it
    // fclose(state->log_file);
    free(state);
}

void create_thread_state_key() {
    pthread_key_create(&thread_state_key, free_state);
}

void set_local_state(LocalState* new_state) {
    LocalState* old_state = pthread_getspecific(thread_state_key);
    if (old_state == NULL) {
        pthread_setspecific(thread_state_key, new_state);
    } else {
        printf("There is already a local state !");
        free(new_state);
        USER_PANIC("How to deal with it");
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
