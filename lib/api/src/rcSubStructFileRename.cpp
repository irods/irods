#include "subStructFileRename.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileRename( rcComm_t *conn, subStructFileRenameInp_t *subStructFileRenameInp )
 *
 * \brief Rename a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileRenameInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileRename( rcComm_t *conn, subStructFileRenameInp_t *subStructFileRenameInp ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_RENAME_AN, subStructFileRenameInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
