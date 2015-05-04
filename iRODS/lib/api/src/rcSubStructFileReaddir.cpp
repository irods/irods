#include "subStructFileReaddir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileReaddir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReaddirInp, rodsDirent_t **rodsDirent )
 *
 * \brief Read a subdirectory within a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileReaddirInp
 * \param[out] rodsDirent
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileReaddir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileReaddirInp,
                        rodsDirent_t **rodsDirent ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_READDIR_AN, subStructFileReaddirInp, NULL,
                             ( void ** ) rodsDirent, NULL );

    return status;
}
