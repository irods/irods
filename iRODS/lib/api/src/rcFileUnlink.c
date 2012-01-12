/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileUnlink.h for a description of this API call.*/

#include "fileUnlink.h"

int
rcFileUnlink (rcComm_t *conn, fileUnlinkInp_t *fileUnlinkInp)
{
    int status;
    status = procApiRequest (conn, FILE_UNLINK_AN,  fileUnlinkInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
