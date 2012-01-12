/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See fileStage.h for a description of this API call.*/

#include "fileStage.h"

int
rcFileStage (rcComm_t *conn, fileStageInp_t *fileStageInp)
{
    int status;
    status = procApiRequest (conn, FILE_STAGE_AN,  fileStageInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
