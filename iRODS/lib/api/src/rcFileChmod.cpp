/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileChmod.h for a description of this API call.*/

#include "fileChmod.h"

int
rcFileChmod (rcComm_t *conn, fileChmodInp_t *fileChmodInp)
{
    int status;
    status = procApiRequest (conn, FILE_CHMOD_AN,  fileChmodInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
