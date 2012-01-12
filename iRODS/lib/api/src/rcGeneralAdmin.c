/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See generalAdmin.h for a description of this API call.*/

#include "generalAdmin.h"

int
rcGeneralAdmin (rcComm_t *conn, generalAdminInp_t *generalAdminInp)
{
    int status;
    status = procApiRequest (conn, GENERAL_ADMIN_AN,  generalAdminInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
