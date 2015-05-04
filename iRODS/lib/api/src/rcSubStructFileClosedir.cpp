#include "subStructFileClosedir.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSubStructFileClosedir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileClosedirInp )
 *
 * \brief Close a sub directory of a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] subStructFileClosedirInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSubStructFileClosedir( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileClosedirInp ) {
    int status;
    status = procApiRequest( conn, SUB_STRUCT_FILE_CLOSEDIR_AN, subStructFileClosedirInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
