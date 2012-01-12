/* This is script-generated code.  */ 
/* See userAdmin.h for a description of this API call.*/

#include "userAdmin.h"

int
rcUserAdmin (rcComm_t *conn, userAdminInp_t *userAdminInp)
{
    int status;
    status = procApiRequest (conn, USER_ADMIN_AN,  userAdminInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
