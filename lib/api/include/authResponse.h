#ifndef AUTH_RESPONSE_H__
#define AUTH_RESPONSE_H__

#include "rcConnect.h"

typedef struct {
    char *response;
    char *username;
} authResponseInp_t;
#define authResponseInp_PI "bin *response(RESPONSE_LEN); str *username;"

#ifdef __cplusplus
extern "C"
#endif
int rcAuthResponse( rcComm_t *conn, authResponseInp_t *authResponseInp );

#endif
