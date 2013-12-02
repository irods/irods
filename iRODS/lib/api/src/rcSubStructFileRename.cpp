/* This is script-generated code.  */ 
/* See subStructFileRename.h for a description of this API call.*/

#include "subStructFileRename.h"

int
rcSubStructFileRename (rcComm_t *conn, subStructFileRenameInp_t *subStructFileRenameInp)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_RENAME_AN, subStructFileRenameInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
