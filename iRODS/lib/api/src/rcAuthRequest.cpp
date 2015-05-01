/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "authRequest.h"

/**
 * \fn rcAuthRequest( rcComm_t *conn, authRequestOut_t **authRequestOut )
 *
 * \brief Begin the authentication handshake with another iRODS server
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
 * \param[out] authRequestOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcAuthRequest( rcComm_t *conn, authRequestOut_t **authRequestOut ) {
    int status;
    status = procApiRequest( conn, AUTH_REQUEST_AN, NULL, NULL,
                             ( void ** ) authRequestOut, NULL );

    return status;
}
