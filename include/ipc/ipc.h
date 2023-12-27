#ifndef __IPC_H__
#define __IPC_H__

#include <common.h>

typedef struct ipc {
    union {
        const char *shrm_path;
    };
} IPC ;

__BEGIN_DECLS

errval_t ipc_init(const char* shrm_path);
void ipc_destroy(void);

__END_DECLS

#endif // __IPC_H__