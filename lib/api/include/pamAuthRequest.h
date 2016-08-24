#ifndef PAM_AUTH_REQUEST_H__
#define PAM_AUTH_REQUEST_H__

#include "rcConnect.h"

typedef struct {
    char *pamUser;
    char *pamPassword;
    int timeToLive;
} pamAuthRequestInp_t;

#define pamAuthRequestInp_PI "str *pamUser; str *pamPassword; int timeToLive;"

typedef struct {
    char *irodsPamPassword;  /* the generated password to use for iRODS */
} pamAuthRequestOut_t;

#define pamAuthRequestOut_PI "str *irodsPamPassword;"

#ifdef __cplusplus
extern "C"
#endif
int rcPamAuthRequest( rcComm_t *conn, pamAuthRequestInp_t *pamAuthRequestInp, pamAuthRequestOut_t **pamAuthRequestOut );

#endif
