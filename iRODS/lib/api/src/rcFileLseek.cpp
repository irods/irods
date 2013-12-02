/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileLseek.h for a description of this API call.*/

#include "fileLseek.h"

int
rcFileLseek (rcComm_t *conn, fileLseekInp_t *fileLseekInp, 
fileLseekOut_t **fileLseekOut)
{
    int status;
    status = procApiRequest (conn, FILE_LSEEK_AN,  fileLseekInp, NULL, 
        (void **) fileLseekOut, NULL);

    return (status);
}
