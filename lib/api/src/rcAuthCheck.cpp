/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "authCheck.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcAuthCheck( rcComm_t *conn, authCheckInp_t *authCheckInp, authCheckOut_t **authCheckOut )
 *
 * \brief Connect to the ICAT-enabled server to verify a user's login
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
 * \param[in] authCheckInp
 * \param[in] authCheckOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcAuthCheck( rcComm_t *conn, authCheckInp_t *authCheckInp,
             authCheckOut_t **authCheckOut ) {
    int status;
    status = procApiRequest( conn, AUTH_CHECK_AN,  authCheckInp, NULL,
                             ( void ** ) authCheckOut, NULL );

    return status;
}
