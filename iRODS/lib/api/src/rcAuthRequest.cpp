/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See authRequest.h for a description of this API call.*/

#include "authRequest.h"

int
rcAuthRequest (rcComm_t *conn, authRequestOut_t **authRequestOut )
{
    int status;
    status = procApiRequest (conn, AUTH_REQUEST_AN, NULL, NULL, 
        (void **) authRequestOut, NULL);

    return (status);
}
