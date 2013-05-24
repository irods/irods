/* This is script-generated code.  */ 
/* See pamAuthRequest.h for a description of this API call.*/

#include "pamAuthRequest.h"

int
rcPamAuthRequest (rcComm_t *conn, pamAuthRequestInp_t *pamAuthRequestInp,
		  pamAuthRequestOut_t **pamAuthRequestOut )
{
    int status;
    status = procApiRequest (conn, PAM_AUTH_REQUEST_AN,  pamAuthRequestInp, NULL, 
        (void **) pamAuthRequestOut, NULL);

    return (status);
}
