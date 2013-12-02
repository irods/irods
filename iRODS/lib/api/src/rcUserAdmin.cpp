/**
 * @file  rcUserAdmin.c
 *
 */
/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
 
/* Also see userAdmin.h for a description of this API call.*/

#include "userAdmin.hpp"

/**
 * \fn rcUserAdmin (rcComm_t *conn, userAdminInp_t *userAdminInp)
 *
 * \brief Perform a user-administrative function.
 *
 * \user client
 *
 * \category metadata operations
 *
 * \since 1.0 (or so)
 *
 * \author  Wayne Schroeder
 * \date    2007 (or so)
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
 * \bug  no known bugs
**/

int
rcUserAdmin (rcComm_t *conn, userAdminInp_t *userAdminInp)
{
    int status;
    status = procApiRequest (conn, USER_ADMIN_AN,  userAdminInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
