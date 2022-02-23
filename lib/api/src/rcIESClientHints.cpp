#include "irods/ies_client_hints.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int
rcIESClientHints( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    status = procApiRequest( conn, IES_CLIENT_HINTS_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
