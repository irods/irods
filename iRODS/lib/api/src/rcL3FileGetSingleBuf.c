/* This is script-generated code.  */ 
/* See l3FileGetSingleBuf.h for a description of this API call.*/

#include "l3FileGetSingleBuf.h"

int
rcL3FileGetSingleBuf (rcComm_t *conn, int l1descInx,
bytesBuf_t *dataObjOutBBuf)
{
    int status;
    status = procApiRequest (conn, L3_FILE_GET_SINGLE_BUF_AN, &l1descInx, NULL,
        (void **) NULL, dataObjOutBBuf);

    return (status);
}


