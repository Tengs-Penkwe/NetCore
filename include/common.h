/* standard libc types and assert */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

/* Error Handling */
#include <errors/errno.h>

/* Debug System */
#include <errors/debug.h>

/* C++ */
#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS	}
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif
