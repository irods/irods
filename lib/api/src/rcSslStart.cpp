/* This is script-generated code.  */
/* See sslStart.h for a description of this API call.*/

#include "irods/sslStart.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int
rcSslStart( rcComm_t *conn, sslStartInp_t *sslStartInp ) {
    int status;
    status = procApiRequest( conn, SSL_START_AN,  sslStartInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
