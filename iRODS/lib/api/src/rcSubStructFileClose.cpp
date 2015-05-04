#include "subStructFileClose.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileClose( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileCloseInp )
 *
 * \brief Close a previously opened subfile of a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileCloseInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileClose( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileCloseInp ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_CLOSE_AN, subStructFileCloseInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
