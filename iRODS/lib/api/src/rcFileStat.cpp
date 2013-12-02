/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileStat.h for a description of this API call.*/

#include "fileStat.h"

int
rcFileStat (rcComm_t *conn, fileStatInp_t *fileStatInp, 
rodsStat_t **fileStatOut)
{
    int status;
    status = procApiRequest (conn, FILE_STAT_AN,  fileStatInp, NULL, 
        (void **) fileStatOut, NULL);

    return (status);
}
