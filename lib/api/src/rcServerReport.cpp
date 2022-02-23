#include "irods/server_report.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int
rcServerReport( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    status = procApiRequest( conn, SERVER_REPORT_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
