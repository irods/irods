/* This is script-generated code.  */ 
/* See fileRename.h for a description of this API call.*/

#include "fileRename.h"

int
rcFileRename (rcComm_t *conn, fileRenameInp_t *fileRenameInp)
{
    int status;
    status = procApiRequest (conn, FILE_RENAME_AN,  fileRenameInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
