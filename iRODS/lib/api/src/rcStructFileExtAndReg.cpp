#include "structFileExtAndReg.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcStructFileExtAndReg( rcComm_t *conn, structFileExtAndRegInp_t *structFileExtAndRegInp )
 *
 * \brief Extract and register a structured file object.
 *
 * \ingroup server_structuredfile
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] structFileExtAndRegInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcStructFileExtAndReg( rcComm_t *conn,
                       structFileExtAndRegInp_t *structFileExtAndRegInp ) {
    int status;
    status = procApiRequest( conn, STRUCT_FILE_EXT_AND_REG_AN,
                             structFileExtAndRegInp, NULL, ( void ** ) NULL, NULL );

    return status;
}
