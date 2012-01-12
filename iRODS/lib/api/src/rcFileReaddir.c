/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileReaddir.h for a description of this API call.*/

#include "fileReaddir.h"

int
rcFileReaddir (rcComm_t *conn, fileReaddirInp_t *fileReaddirInp, 
rodsDirent_t **fileReaddirOut)
{
    int status;
    status = procApiRequest (conn, FILE_READDIR_AN,  fileReaddirInp, NULL, 
        (void **) fileReaddirOut, NULL);

    return (status);
}
