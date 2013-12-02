/* This is script-generated code.  */ 
/* See bunSubMkdir.h for a description of this API call.*/

#include "subStructFileMkdir.h"

int
rcSubStructFileMkdir (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_MKDIR_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
