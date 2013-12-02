/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileRmdir.h for a description of this API call.*/

#include "fileRmdir.h"

int
rcFileRmdir (rcComm_t *conn, fileRmdirInp_t *fileRmdirInp)
{
    int status;
    status = procApiRequest (conn, FILE_RMDIR_AN,  fileRmdirInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
