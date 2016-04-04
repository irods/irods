
#include "client_hints.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
rcClientHints( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    printf( "calling CLIENT_HINTS_AN - %d", CLIENT_HINTS_AN );
    status = procApiRequest( conn, CLIENT_HINTS_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
