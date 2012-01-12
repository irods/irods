/* This is script-generated code.  */ 
/* See fileTruncate.h for a description of this API call.*/

#include "fileTruncate.h"

int
rcFileTruncate (rcComm_t *conn, fileOpenInp_t *fileTruncateInp)
{
    int status;
    status = procApiRequest (conn, FILE_TRUNCATE_AN, fileTruncateInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
