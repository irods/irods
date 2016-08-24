#ifndef AUTH_CHECK_H__
#define AUTH_CHECK_H__

#include "rcConnect.h"

typedef struct {
    char *challenge;
    char *response;
    char *username;
} authCheckInp_t;

typedef struct {
    int  privLevel;
    int  clientPrivLevel;
    char *serverResponse;
} authCheckOut_t;

#define authCheckInp_PI "str *challenge; str *response; str *username;"
#define authCheckOut_PI "int privLevel; int clientPrivLevel; str *serverResponse;"


#ifdef __cplusplus
extern "C"
#endif
int rcAuthCheck( rcComm_t *conn, authCheckInp_t *authCheckInp, authCheckOut_t **authCheckOut );

#endif
