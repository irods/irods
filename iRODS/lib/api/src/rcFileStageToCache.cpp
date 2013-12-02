/* This is script-generated code.  */ 
/* See fileStageToCache.h for a description of this API call.*/

#include "fileStageToCache.h"

int
rcFileStageToCache (rcComm_t *conn, fileStageSyncInp_t *fileStageToCacheInp)
{
    int status;
    status = procApiRequest (conn, FILE_STAGE_TO_CACHE_AN, 
      fileStageToCacheInp, NULL, (void **) NULL, NULL);

    return (status);
}
