/**
 * @file  rcGeneralAdmin.cpp
 *
 */
/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "generalAdmin.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGeneralAdmin (rcComm_t *conn, generalAdminInp_t *generalAdminInp)
 *
 * \brief Perform a general-administrative function.
 *
 * \user client (the API is restricted to admin users only)
 *
 * \ingroup administration
 *
 * \since .5
 *
 *
 * \remark Perform a administrative function.
 * \n This is used extensively from iadmin.
 * \n The input is a set of text strings to be parsed on the server side.
 * \n This allows this same API to be easily extended with new functionality
 * \n and well managed backward compatability.  See rsGeneralAdmin.cpp (and
 * \n iadmin.c) for how this is parsed and how it interfaces to the
 * \n catalog-high-level (chl*) functions.
 * \n Primary results are changes to rows in the ICAT database.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] generalAdminInp - input general-admin structure
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/

int
rcGeneralAdmin( rcComm_t *conn, generalAdminInp_t *generalAdminInp ) {
    int status;
    status = procApiRequest( conn, GENERAL_ADMIN_AN,  generalAdminInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
