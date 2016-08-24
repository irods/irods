#ifndef USER_ADMIN_H__
#define USER_ADMIN_H__

#include "rcConnect.h"

typedef struct {
    char *arg0;
    char *arg1;
    char *arg2;
    char *arg3;
    char *arg4;
    char *arg5;
    char *arg6;
    char *arg7;
    char *arg8;
    char *arg9;
} userAdminInp_t;
#define userAdminInp_PI "str *arg0; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7;  str *arg8;  str *arg9;"

#ifdef __cplusplus
extern "C"
#endif
int rcUserAdmin( rcComm_t *conn, userAdminInp_t *userAdminInp );

#endif
