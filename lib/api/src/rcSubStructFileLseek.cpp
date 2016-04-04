#include "subStructFileLseek.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileLseek( rcComm_t *conn, subStructFileLseekInp_t *subStructFileLseekInp, fileLseekOut_t **subStructFileLseekOut )
 *
 * \brief Seek within a subfile of a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileLseekInp
 * \param[out] subStructFileLseekOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileLseek( rcComm_t *conn, subStructFileLseekInp_t *subStructFileLseekInp,
                      fileLseekOut_t **subStructFileLseekOut ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_LSEEK_AN, subStructFileLseekInp, NULL,
                             ( void ** ) subStructFileLseekOut, NULL );

    return status;
}
