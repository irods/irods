#ifndef AUTH_REQUEST_H__
#define AUTH_REQUEST_H__

#include "irods/rods.h"
#include "irods/rcMisc.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"
#include "irods/authenticate.h"
#include "irods/icatDefines.h"

typedef struct {
    char *challenge;
} authRequestOut_t;

#define authRequestOut_PI "bin *challenge(CHALLENGE_LEN);"

#ifdef __cplusplus
extern "C"
#endif
int rcAuthRequest( rcComm_t *conn, authRequestOut_t **authRequestOut );

#endif
