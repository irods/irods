/* This is script-generated code.  */
/* See sslEnd.h for a description of this API call.*/

#include "irods/sslEnd.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int
rcSslEnd( rcComm_t *conn, sslEndInp_t *sslEndInp ) {
    int status;
    status = procApiRequest( conn, SSL_END_AN,  sslEndInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
