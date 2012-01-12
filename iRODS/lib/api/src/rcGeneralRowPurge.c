/* This is script-generated code.  */ 
/* See generalRowPurge.h for a description of this API call.*/

#include "generalRowPurge.h"

int
rcGeneralRowPurge (rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp)
{
    int status;
    status = procApiRequest (conn, GENERAL_ROW_PURGE_AN,  generalRowPurgeInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
