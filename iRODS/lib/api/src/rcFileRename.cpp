/* This is script-generated code.  */
/* See fileRename.h for a description of this API call.*/

#include "fileRename.hpp"

int
rcFileRename( rcComm_t *conn, fileRenameInp_t *fileRenameInp, fileRenameOut_t** _out ) {
    int status;
    status = procApiRequest( conn, FILE_RENAME_AN,  fileRenameInp, NULL,
                             ( void ** ) _out, NULL );

    return ( status );
}
