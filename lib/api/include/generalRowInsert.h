#ifndef GENERAL_ROW_INSERT_H__
#define GENERAL_ROW_INSERT_H__

#include "rcConnect.h"

typedef struct {
    const char *tableName;
    const char *arg1;
    const char *arg2;
    const char *arg3;
    const char *arg4;
    const char *arg5;
    const char *arg6;
    const char *arg7;
    const char *arg8;
    const char *arg9;
    const char *arg10;
} generalRowInsertInp_t;
#define generalRowInsertInp_PI "str *tableName; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9;"


#ifdef __cplusplus
extern "C"
#endif
int rcGeneralRowInsert( rcComm_t *conn, generalRowInsertInp_t *generalRowInsertInp );

#endif
