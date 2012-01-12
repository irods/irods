/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileFstat.h for a description of this API call.*/

#include "fileFstat.h"

int
rcFileFstat (rcComm_t *conn, fileFstatInp_t *fileFstatInp, 
rodsStat_t **fileFstatOut)
{
    int status;
    status = procApiRequest (conn, FILE_FSTAT_AN,  fileFstatInp, NULL, 
        (void **) fileFstatOut, NULL);

    return (status);
}
