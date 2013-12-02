/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileClosedir.h for a description of this API call.*/

#include "fileClosedir.h"

int
rcFileClosedir (rcComm_t *conn, fileClosedirInp_t *fileClosedirInp)
{
    int status;
    status = procApiRequest (conn, FILE_CLOSEDIR_AN,  fileClosedirInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
