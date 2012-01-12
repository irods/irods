/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileClose.h for a description of this API call.*/

#include "fileClose.h"

int
rcFileClose (rcComm_t *conn, fileCloseInp_t *fileCloseInp)
{
    int status;
    status = procApiRequest (conn, FILE_CLOSE_AN,  fileCloseInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
