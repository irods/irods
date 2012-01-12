/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileChksum.h for a description of this API call.*/

#include "fileChksum.h"

int
rcFileChksum (rcComm_t *conn, fileChksumInp_t *fileChksumInp,
char **chksumStr)
{
    int status;
    status = procApiRequest (conn, FILE_CHKSUM_AN,  fileChksumInp, NULL, 
        (void **) chksumStr, NULL);

    return (status);
}
