/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileGetFsFreeSpace.h for a description of this API call.*/

#include "fileGetFsFreeSpace.h"

int
rcFileGetFsFreeSpace (rcComm_t *conn, 
fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut)
{
    int status;
    status = procApiRequest (conn, FILE_GET_FS_FREE_SPACE_AN,  
      fileGetFsFreeSpaceInp, NULL, (void **) fileGetFsFreeSpaceOut, NULL);

    return (status);
}
