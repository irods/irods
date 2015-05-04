#include "subStructFileTruncate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileTruncate( rcComm_t *conn, subFile_t *bunSubTruncateInp )
 *
 * \brief Truncate a subfile within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] bunSubTruncateInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileTruncate( rcComm_t *conn, subFile_t *bunSubTruncateInp ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_TRUNCATE_AN, bunSubTruncateInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}

