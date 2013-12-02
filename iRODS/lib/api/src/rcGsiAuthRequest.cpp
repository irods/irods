/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See gsiAuthRequest.h for a description of this API call.*/

#include "gsiAuthRequest.h"

int
rcGsiAuthRequest (rcComm_t *conn, gsiAuthRequestOut_t **gsiAuthRequestOut )
{
    int status;
    status = procApiRequest (conn, GSI_AUTH_REQUEST_AN, NULL, NULL, 
        (void **) gsiAuthRequestOut, NULL);

    return (status);
}
