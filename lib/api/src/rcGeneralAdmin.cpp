#include "irods/generalAdmin.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

int
rcGeneralAdmin( rcComm_t *conn, generalAdminInp_t *generalAdminInp ) {
    int status;
    status = procApiRequest( conn, GENERAL_ADMIN_AN,  generalAdminInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
