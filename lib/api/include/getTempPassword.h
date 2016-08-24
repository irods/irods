#ifndef GET_TEMP_PASSWORD_H__
#define GET_TEMP_PASSWORD_H__

#include "rcConnect.h"
#include "authenticate.h"

typedef struct {
    char stringToHashWith[MAX_PASSWORD_LEN];
} getTempPasswordOut_t;
#define getTempPasswordOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"

int rcGetTempPassword( rcComm_t *conn, getTempPasswordOut_t **getTempPasswordOut );

#endif
