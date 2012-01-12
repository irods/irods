/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileFsync.h for a description of this API call.*/

#include "fileFsync.h"

int
rcFileFsync (rcComm_t *conn, fileFsyncInp_t *fileFsyncInp)
{
    int status;
    status = procApiRequest (conn, FILE_FSYNC_AN,  fileFsyncInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
