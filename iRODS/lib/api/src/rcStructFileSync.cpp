/* This is script-generated code.  */ 
/* See structFileSync.h for a description of this API call.*/

#include "structFileSync.h"

int
rcStructFileSync (rcComm_t *conn, structFileOprInp_t *structFileOprInp)
{
    int status;
    status = procApiRequest (conn, STRUCT_FILE_SYNC_AN, structFileOprInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
