/* This is script-generated code.  */ 
/* See streamRead.h for a description of this API call.*/

#include "streamRead.h"

int
rcStreamRead (rcComm_t *conn, fileReadInp_t *streamReadInp,
bytesBuf_t *streamReadOutBBuf)
{
    int status;
    status = procApiRequest (conn, STREAM_READ_AN, streamReadInp, NULL, 
        (void **) NULL, streamReadOutBBuf);

    return (status);
}
