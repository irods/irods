/* This is script-generated code.  */ 
/* See bunSubRmdir.h for a description of this API call.*/

#include "subStructFileRmdir.h"

int
rcSubStructFileRmdir (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_RMDIR_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
