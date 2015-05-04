#include "ies_client_hints.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
rcIESClientHints( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    status = procApiRequest( conn, IES_CLIENT_HINTS_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
