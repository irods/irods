/* This is script-generated code.  */ 
/* See subStructFileStat.h for a description of this API call.*/

#include "subStructFileStat.h"

int
rcSubStructFileStat (rcComm_t *conn, subFile_t *subFile,
rodsStat_t **subStructFileStatOut)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_STAT_AN, subFile, NULL, 
        (void **) subStructFileStatOut, NULL);

    return (status);
}
