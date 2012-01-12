/* This is script-generated code.  */ 
/* See bunSubOpen.h for a description of this API call.*/

#include "subStructFileOpen.h"

int
rcSubStructFileOpen (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_OPEN_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
