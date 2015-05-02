/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "authResponse.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcAuthResponse( rcComm_t *conn, authResponseInp_t *authResponseInp )
 *
 * \brief Response of the authentication handshake with another iRODS server
 *
 * \user client
 *
 * \ingroup authentication
 *
 * \since 1.0
 *
 *
 * \remark This is a Metadata API call, is only used server to server
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] authResponseInp
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcAuthResponse( rcComm_t *conn, authResponseInp_t *authResponseInp ) {
    int status;
    status = procApiRequest( conn, AUTH_RESPONSE_AN,  authResponseInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
