/* This is script-generated code.  */ 
/* See sslStart.h for a description of this API call.*/

#include "sslStart.h"

int
rcSslStart (rcComm_t *conn, sslStartInp_t *sslStartInp)
{
    int status;
    status = procApiRequest (conn, SSL_START_AN,  sslStartInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
