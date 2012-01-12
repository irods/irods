/* This is script-generated code.  */ 
/* See l3FilePutSingleBuf.h for a description of this API call.*/

#include "l3FilePutSingleBuf.h"

int
rcL3FilePutSingleBuf (rcComm_t *conn, int l1descInx,
bytesBuf_t *dataObjInBBuf)
{
    int status;
    status = procApiRequest (conn, L3_FILE_PUT_SINGLE_BUF_AN, 
      &l1descInx, dataObjInBBuf, (void **) NULL, NULL);

    return (status);
}


