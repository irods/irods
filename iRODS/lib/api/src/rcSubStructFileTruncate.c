/* This is script-generated code.  */ 
/* See bunSubTruncate.h for a description of this API call.*/

#include "subStructFileTruncate.h"

int
rcSubStructFileTruncate (rcComm_t *conn, subFile_t *bunSubTruncateInp)
{
    int status;
    status = procApiRequest (conn, SUB_STRUCT_FILE_TRUNCATE_AN, bunSubTruncateInp, 
      NULL, (void **) NULL, NULL);

    return (status);
}
