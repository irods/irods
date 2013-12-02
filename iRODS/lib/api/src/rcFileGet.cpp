/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileGet.h for a description of this API call.*/

#include "fileGet.h"

int
rcFileGet (rcComm_t *conn, fileOpenInp_t *fileGetInp, 
bytesBuf_t *fileGetOutBBuf)
{
    int status;
    status = procApiRequest (conn, FILE_GET_AN, fileGetInp, NULL, 
        (void **) NULL, fileGetOutBBuf);

    return (status);
}
