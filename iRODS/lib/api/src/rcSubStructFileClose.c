/* This is script-generated code.  */ 
/* See subStructFileClose.h for a description of this API call.*/

#include "subStructFileClose.h"

int
rcSubStructFileClose (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileCloseInp)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_CLOSE_AN, subStructFileCloseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
