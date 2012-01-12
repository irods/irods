/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileMkdir.h for a description of this API call.*/

#include "fileMkdir.h"

int
rcFileMkdir (rcComm_t *conn, fileMkdirInp_t *fileMkdirInp)
{
    int status;
    status = procApiRequest (conn, FILE_MKDIR_AN,  fileMkdirInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
