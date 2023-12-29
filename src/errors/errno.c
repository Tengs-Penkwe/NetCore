#include <errors/errno.h>
#include <stdio.h>      // sprintf
#include <unistd.h>     // write

const char* err_code_strings[] = {
#define X(code, str) str,
    OK_CODES
    THROW_CODES
    SYSTEM_ERR_CODES
    EVENT_ERR_CODES
    NETWORK_ERR_CODES
    IPC_ERR_CODES
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

void err_print_calltrace(errval_t err, FILE* log){
    enum err_code x = err_no(err);
    while((x = err_no(err)) != SYS_ERR_OK)
    { 
        char err_string[128];
        int len = sprintf(err_string, "Failure: ( %32s )\n", err_getstring(x));
        fwrite(err_string, sizeof(char), len, log);
        err = err_pop(err);
    }       
}