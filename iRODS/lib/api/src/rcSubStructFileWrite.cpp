/* This is script-generated code.  */ 
/* See subStructFileWrite.h for a description of this API call.*/

#include "subStructFileWrite.h"

int
rcSubStructFileWrite (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileWriteInp,
bytesBuf_t *subStructFileWriteOutBBuf)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_WRITE_AN, subStructFileWriteInp, 
     subStructFileWriteOutBBuf, (void **) NULL, NULL);

    return (status);
}
