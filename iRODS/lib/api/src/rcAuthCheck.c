/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See authCheck.h for a description of this API call.*/

#include "authCheck.h"

int
rcAuthCheck (rcComm_t *conn, authCheckInp_t *authCheckInp, 
	     authCheckOut_t **authCheckOut )
{
    int status;
    status = procApiRequest (conn, AUTH_CHECK_AN,  authCheckInp, NULL, 
        (void **) authCheckOut, NULL);

    return (status);
}
