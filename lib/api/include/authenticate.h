#ifndef AUTHENTICATE_H__
#define AUTHENTICATE_H__

#include "rcConnect.h"

#define AUTH_SUBOP_REQ_AUTH "request challenge"
#define AUTH_SUBOP_RESP "challenge response"

#define MAX_PASSWORD_LEN 50
#define CHALLENGE_LEN 64    // 64 bytes of data and terminating null
#define RESPONSE_LEN 16     // 16 bytes of data and terminating null


typedef struct {
    char *subOp;
    char *response;
    char *username;
} AuthenticateInp_t;
#define AuthenticateInp_PI "str *subOp; str *response; str *username;"

typedef struct {
    char *challenge;
} AuthenticateOut_t;
#define AuthenticateOut_PI "str *challenge;"


#ifdef __cplusplus
extern "C"
#endif
int rcAuthenticate( rcComm_t *conn, AuthenticateInp_t *authenticateInp, AuthenticateOut_t **authenticateOut );

#endif
