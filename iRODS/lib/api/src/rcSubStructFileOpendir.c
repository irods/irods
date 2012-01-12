/* This is script-generated code.  */ 
/* See bunSubOpendir.h for a description of this API call.*/

#include "subStructFileOpendir.h"

int
rcSubStructFileOpendir (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_OPENDIR_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
