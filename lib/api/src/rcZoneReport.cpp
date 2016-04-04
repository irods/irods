#include "zone_report.h"
#include "procApiRequest.h"
#include "apiNumber.h"

int
rcZoneReport( rcComm_t *conn, bytesBuf_t** _bbuf ) {
    int status;
    status = procApiRequest( conn, ZONE_REPORT_AN, NULL, NULL,
                             ( void ** ) _bbuf, NULL );

    return status;
}
