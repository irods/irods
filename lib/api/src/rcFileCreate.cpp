#include "fileCreate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcFileCreate( rcComm_t *conn, fileCreateInp_t *fileCreateInp, fileCreateOut_t** _out )
 *
 * \brief Register a data object.
 *
 * \ingroup server_filedriver
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] fileCreateInp
 * \param[out] _out
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcFileCreate( rcComm_t *conn, fileCreateInp_t *fileCreateInp, fileCreateOut_t** _out ) {
    int status;

    status = procApiRequest( conn, FILE_CREATE_AN, fileCreateInp, NULL,
                             ( void ** ) _out, NULL );

    return status;
}

