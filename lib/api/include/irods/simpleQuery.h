#ifndef SIMPLE_QUERY_H__
#define SIMPLE_QUERY_H__

#include "irods/rcConnect.h"

typedef struct {
    char *sql;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    int control;
    int form;
    int maxBufSize;
} simpleQueryInp_t;
#define simpleQueryInp_PI "str *sql; str *arg1; str *arg2; str *arg3; str *arg4; int control; int form; int maxBufSize;"

typedef struct {
    int control;
    char *outBuf;
} simpleQueryOut_t;
#define simpleQueryOut_PI "int control; str *outBuf;"


#ifdef __cplusplus
extern "C"
#endif
__attribute__((deprecated("SimpleQuery is deprecated. Use GenQuery or SpecificQuery instead.")))
int rcSimpleQuery(rcComm_t* conn, simpleQueryInp_t* simpleQueryInp, simpleQueryOut_t** simpleQueryOut);

#endif
