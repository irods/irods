/* This is script-generated code.  */ 
/* See modAccessControl.h for a description of this API call.*/

#include "modAccessControl.h"

int
rcModAccessControl (rcComm_t *conn, modAccessControlInp_t *modAccessControlInp)
{
    int status;
    status = procApiRequest (conn, MOD_ACCESS_CONTROL_AN,  modAccessControlInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
