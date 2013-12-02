/* This is script-generated code.  */ 
/* See streamClose.h for a description of this API call.*/

#include "streamClose.h"

int
rcStreamClose (rcComm_t *conn, fileCloseInp_t *fileCloseInp)
{
    int status;
    status = procApiRequest (conn, STREAM_CLOSE_AN, fileCloseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
