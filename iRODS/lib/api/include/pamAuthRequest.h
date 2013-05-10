/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* pamAuthRequest.h
 */

#ifndef PAM_AUTH_REQUEST_H
#define PAM_AUTH_REQUEST_H

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
   char *pamUser;
   char *pamPassword;
   int timeToLive;
} pamAuthRequestInp_t;
    
#define pamAuthRequestInp_PI "str *pamUser; str *pamPassword; int timeToLive;"

typedef struct {
   char *irodsPamPassword;  /* the generated password to use for iRODS */
} pamAuthRequestOut_t;
    
#define pamAuthRequestOut_PI "str *irodsPamPassword;"


#if defined(RODS_SERVER)
#define RS_PAM_AUTH_REQUEST rsPamAuthRequest
/* prototype for the server handler */
int
rsPamAuthRequest (rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp,
		  pamAuthRequestOut_t **pamAuthRequestOut );
int
_rsPamAuthRequest (rsComm_t *rsComm, pamAuthRequestInp_t *pamAuthRequestInp,
		  pamAuthRequestOut_t **pamAuthRequestOut );
#else
#define RS_PAM_AUTH_REQUEST NULL
#endif

/* prototype for the client call */
int
rcPamAuthRequest (rcComm_t *conn, pamAuthRequestInp_t *pamAuthRequestInp,
		  pamAuthRequestOut_t **pamAuthRequestOut );

#ifdef  __cplusplus
}
#endif

#endif	/* PAM_AUTH_REQUEST_H */
