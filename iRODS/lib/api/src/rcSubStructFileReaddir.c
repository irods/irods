/* This is script-generated code.  */ 
/* See subStructFileReaddir.h for a description of this API call.*/

#include "subStructFileReaddir.h"

int
rcSubStructFileReaddir (rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReaddirInp,
rodsDirent_t **rodsDirent)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_READDIR_AN, subStructFileReaddirInp, NULL, 
        (void **) rodsDirent, NULL);

    return (status);
}
