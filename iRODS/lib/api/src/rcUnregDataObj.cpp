/* This is script-generated code.  */ 
/* See unregDataObj.h for a description of this API call.*/

#include "unregDataObj.h"

int
rcUnregDataObj (rcComm_t *conn, unregDataObj_t *unregDataObjInp)
{
    int status;
    status = procApiRequest (conn, UNREG_DATA_OBJ_AN, unregDataObjInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
