/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* gsiAuthRequest.h
 */

#ifndef GSI_AUTH_REQUEST_HPP
#define GSI_AUTH_REQUEST_HPP

/* This is a Metadata API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "icatDefines.hpp"

typedef struct {
    char *serverDN;
} gsiAuthRequestOut_t;

#define gsiAuthRequestOut_PI "str *ServerDN;"


#if defined(RODS_SERVER)
#define RS_GSI_AUTH_REQUEST rsGsiAuthRequest
/* prototype for the server handler */
int
rsGsiAuthRequest( rsComm_t *rsComm, gsiAuthRequestOut_t **gsiAuthRequestOut );

#else
#define RS_GSI_AUTH_REQUEST NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /* prototype for the client call */
    int
    rcGsiAuthRequest( rcComm_t *conn, gsiAuthRequestOut_t **gsiAuthRequestOut );

#ifdef __cplusplus
}
#endif
#endif	/* GSI_AUTH_REQUEST_H */
