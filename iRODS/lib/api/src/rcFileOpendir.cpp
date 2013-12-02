/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileOpendir.h for a description of this API call.*/

#include "fileOpendir.h"

int
rcFileOpendir (rcComm_t *conn, fileOpendirInp_t *fileOpendirInp)
{
    int status;
    status = procApiRequest (conn, FILE_OPENDIR_AN,  fileOpendirInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
