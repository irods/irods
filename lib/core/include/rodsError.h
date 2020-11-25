#ifndef RODS_ERROR_H__
#define RODS_ERROR_H__

#define USE_EXTERN_TABLE

#define ERR_MSG_LEN             1024
#define MAX_ERROR_MESSAGES      100

typedef struct ErrorMessage {
    int status;
    char msg[ERR_MSG_LEN];
} rErrMsg_t;

typedef struct ErrorStack {
    int len;            // Number of errors in the stack.
    rErrMsg_t **errMsg; // An array of pointers to the ErrorMessage struct.
} rError_t;

#endif // RODS_ERROR_H__
