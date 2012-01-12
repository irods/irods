/* This is script-generated code.  */ 
/* See subStructFilePut.h for a description of this API call.*/

#include "subStructFilePut.h"

int
rcSubStructFilePut (rcComm_t *conn, subFile_t *subFile, 
bytesBuf_t *subFilePutOutBBuf)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_PUT_AN, subFile, 
      subFilePutOutBBuf, (void **) NULL, NULL);

    return (status);
}
