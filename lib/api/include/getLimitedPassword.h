#ifndef GET_LIMITED_PASSWORD_H__
#define GET_LIMITED_PASSWORD_H__

#include "rcConnect.h"
#include "authenticate.h"

typedef struct {
    int  ttl;
    char *unused1;  // currently unused, but available without protocol change if needed
} getLimitedPasswordInp_t;
#define getLimitedPasswordInp_PI "int ttl; str *unused1;"

typedef struct {
    char stringToHashWith[MAX_PASSWORD_LEN];
} getLimitedPasswordOut_t;
#define getLimitedPasswordOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"


int rcGetLimitedPassword( rcComm_t *conn, getLimitedPasswordInp_t *getLimitedPasswordInp, getLimitedPasswordOut_t **getLimitedPasswordOut );

#endif
