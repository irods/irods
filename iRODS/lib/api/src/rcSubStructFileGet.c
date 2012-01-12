/* This is script-generated code.  */ 
/* See subStructFileGet.h for a description of this API call.*/

#include "subStructFileGet.h"

int
rcSubStructFileGet (rcComm_t *conn, subFile_t *subFile, 
bytesBuf_t *subFileGetOutBBuf)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_GET_AN, subFile, NULL, 
        (void **) NULL, subFileGetOutBBuf);

    return (status);
}
