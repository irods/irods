/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* authResponse.h
 */

#ifndef AUTH_RESPONSE_H
#define AUTH_RESPONSE_H

/* This is a Metadata API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
   char *response;
   char *username;
} authResponseInp_t;
    
#define authResponseInp_PI "bin *response(RESPONSE_LEN); str *username;"

#if defined(RODS_SERVER)
#define RS_AUTH_RESPONSE rsAuthResponse
/* prototype for the server handler */
int
rsAuthResponse (rsComm_t *rsComm, authResponseInp_t *authResponseInp );
int
chkProxyUserPriv (rsComm_t *rsComm, int proxyUserPriv);
#else
#define RS_AUTH_RESPONSE NULL
#endif

/* prototype for the client call */
int
rcAuthResponse (rcComm_t *conn, authResponseInp_t *authResponseInp );

#ifdef  __cplusplus
}
#endif

#endif	/* AUTH_RESPONSE_H */
