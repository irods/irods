/* This is script-generated code.  */ 
/* See generalUpdate.h for a description of this API call.*/

#include "generalUpdate.h"

int
rcGeneralUpdate (rcComm_t *conn, generalUpdateInp_t *generalUpdateInp)
{
    int status;
    status = procApiRequest (conn, GENERAL_UPDATE_AN, generalUpdateInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
