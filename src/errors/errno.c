#include <errors/errno.h>

const char* err_code_strings[] = {
#define X(code, str) str,
    SYSTEM_ERR_CODES
    NETWORK_ERR_CODES
#undef X
};

const char* err_code_to_string(enum err_code code) {
    if (code < sizeof(err_code_strings) / sizeof(err_code_strings[0])) {
        return err_code_strings[code];
    }
    return "Unknown Error Code";
}

char* err_getstring(errval_t errval) {
    enum err_code code = err_no(errval);
    return (char*)err_code_to_string(code);
}
