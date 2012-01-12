/* This is script-generated code.  */ 
/* See krbAuthRequest.h for a description of this API call.*/

#include "krbAuthRequest.h"

int
rcKrbAuthRequest (rcComm_t *conn, krbAuthRequestOut_t **krbAuthRequestOut )
{
    int status;
    status = procApiRequest (conn, KRB_AUTH_REQUEST_AN, NULL, NULL, 
        (void **) krbAuthRequestOut, NULL);

    return (status);
}
