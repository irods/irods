/* This is script-generated code.  */
/* See getTempPassword.h for a description of this API call.*/

#include "server_report.hpp"

int
rcServerReport( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    status = procApiRequest( conn, SERVER_REPORT_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
