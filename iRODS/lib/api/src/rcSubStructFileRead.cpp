/* This is script-generated code.  */ 
/* See subStructFileRead.h for a description of this API call.*/

#include "subStructFileRead.hpp"

int
rcSubStructFileRead (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReadInp,
bytesBuf_t *subStructFileReadOutBBuf)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_READ_AN, subStructFileReadInp, NULL, 
        (void **) NULL, subStructFileReadOutBBuf);

    return (status);
}
