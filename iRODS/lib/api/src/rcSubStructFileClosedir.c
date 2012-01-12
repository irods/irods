/* This is script-generated code.  */ 
/* See subStructFileClosedir.h for a description of this API call.*/

#include "subStructFileClosedir.h"

int
rcSubStructFileClosedir (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileClosedirInp)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_CLOSEDIR_AN, subStructFileClosedirInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
