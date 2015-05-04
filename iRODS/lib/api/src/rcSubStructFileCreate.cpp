#include "subStructFileCreate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileCreate( rcComm_t *conn, subFile_t *subFile )
 *
 * \brief Create a subfile of a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subFile
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileCreate( rcComm_t *conn, subFile_t *subFile ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_CREATE_AN, subFile, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
