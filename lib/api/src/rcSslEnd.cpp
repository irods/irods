/* This is script-generated code.  */
/* See sslEnd.h for a description of this API call.*/

#include "sslEnd.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
rcSslEnd( rcComm_t *conn, sslEndInp_t *sslEndInp ) {
    int status;
    status = procApiRequest( conn, SSL_END_AN,  sslEndInp, nullptr,
                             ( void ** ) nullptr, nullptr );

    return status;
}
