#ifndef AUTH_REQUEST_H__
#define AUTH_REQUEST_H__

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "authenticate.h"
#include "icatDefines.h"

typedef struct {
    char *challenge;
} authRequestOut_t;

#define authRequestOut_PI "bin *challenge(CHALLENGE_LEN);"

#ifdef __cplusplus
extern "C"
#endif
int rcAuthRequest( rcComm_t *conn, authRequestOut_t **authRequestOut );

#endif
