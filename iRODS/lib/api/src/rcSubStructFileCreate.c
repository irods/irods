/* This is script-generated code.  */ 
/* See bunSubCreate.h for a description of this API call.*/

#include "subStructFileCreate.h"

int
rcSubStructFileCreate (rcComm_t *conn, subFile_t *subFile)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_CREATE_AN, subFile, NULL, 
        (void **) NULL, NULL);

    return (status);
}
