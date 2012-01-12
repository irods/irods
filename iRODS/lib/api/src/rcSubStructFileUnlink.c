/* This is script-generated code.  */ 
/* See bunSubUnlink.h for a description of this API call.*/

#include "subStructFileUnlink.h"

int
rcSubStructFileUnlink (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_UNLINK_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
