/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See authResponse.h for a description of this API call.*/

#include "authResponse.h"

int
rcAuthResponse (rcComm_t *conn, authResponseInp_t *authResponseInp )
{
    int status;
    status = procApiRequest (conn, AUTH_RESPONSE_AN,  authResponseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
