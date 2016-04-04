#include "fileRename.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileRename( rcComm_t *conn, fileRenameInp_t *fileRenameInp, fileRenameOut_t** _out )
 *
 * \brief Renames a file.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileRenameInp
 * \param[out] _out - the output
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileRename( rcComm_t *conn, fileRenameInp_t *fileRenameInp, fileRenameOut_t** _out ) {
    int status;
    status = procApiRequest( conn, FILE_RENAME_AN,  fileRenameInp, NULL,
                             ( void ** ) _out, NULL );

    return status;
}
