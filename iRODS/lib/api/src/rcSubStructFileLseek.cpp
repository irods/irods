/* This is script-generated code.  */ 
/* See subStructFileLseek.h for a description of this API call.*/

#include "subStructFileLseek.h"

int
rcSubStructFileLseek (rcComm_t *conn, subStructFileLseekInp_t *subStructFileLseekInp,
fileLseekOut_t **subStructFileLseekOut)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_LSEEK_AN, subStructFileLseekInp, NULL, 
        (void **) subStructFileLseekOut, NULL);

    return (status);
}
