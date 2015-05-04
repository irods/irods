/**
 * @file  rcUserAdmin.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "userAdmin.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcUserAdmin (rcComm_t *conn, userAdminInp_t *userAdminInp)
 *
 * \brief Perform a user-administrative function.
 *
 * \user client
 *
 * \ingroup administration
 *
 * \since 1.0 (or so)
 *
 *
 * \remark Perform a user administrative function.
 * \n This is similar to general-admin but is for the regular, non-admin, users.
 * \n The input is a set of text strings to be parsed on the server side.
 * \n This allows this same API to be easily extended with new functionality
 * \n and well managed backward compatability.  This is used, for example,
 * \n to allow users to change their passwords (ipasswd).
 * \n Primary results are changes to rows in the ICAT database.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] userAdminInp - input user-admin structure
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcUserAdmin( rcComm_t *conn, userAdminInp_t *userAdminInp ) {
    int status;
    status = procApiRequest( conn, USER_ADMIN_AN,  userAdminInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
