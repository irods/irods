/* This is script-generated code.  */ 
/* See subStructFileFstat.h for a description of this API call.*/

#include "subStructFileFstat.h"

int
rcSubStructFileFstat (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileFstatInp,
rodsStat_t **subStructFileStatOut)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_FSTAT_AN, subStructFileFstatInp, NULL, 
        (void **) subStructFileStatOut, NULL);

    return (status);
}
