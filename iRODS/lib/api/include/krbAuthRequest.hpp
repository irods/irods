/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* krbAuthRequest.h
 */

#ifndef KRB_AUTH_REQUEST_HPP
#define KRB_AUTH_REQUEST_HPP

/* This is a Metadata API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "icatDefines.hpp"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
   char *serverName;
} krbAuthRequestOut_t;
    
#define krbAuthRequestOut_PI "str *ServerName;"


#if defined(RODS_SERVER)
#define RS_KRB_AUTH_REQUEST rsKrbAuthRequest
/* prototype for the server handler */
int
rsKrbAuthRequest (rsComm_t *rsComm, krbAuthRequestOut_t **krbAuthRequestOut );

#else
#define RS_KRB_AUTH_REQUEST NULL
#endif

/* prototype for the client call */
int
rcKrbAuthRequest (rcComm_t *conn, krbAuthRequestOut_t **krbAuthRequestOut );

#ifdef  __cplusplus
}
#endif

#endif	/* KRB_AUTH_REQUEST_H */
