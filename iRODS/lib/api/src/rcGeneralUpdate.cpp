/* This is script-generated code.  */
/* See generalUpdate.h for a description of this API call.*/

#include "generalUpdate.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGeneralUpdate( rcComm_t *conn, generalUpdateInp_t *generalUpdateInp )
 *
 * \brief Add or remove rows from tables. Admin only.
 *
 * \user client (the API is restricted to admin users only)
 *
 * \ingroup administration
 *
 * \since .5
 *
 *
 * \remark    This call performs either a generalInsert or generalDelete call,
 * which can add or remove specified rows from tables using input
 * parameters similar to the generalQuery.  This is restricted to
 * irods-admin users.

 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] generalUpdateInp
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/
int
rcGeneralUpdate( rcComm_t *conn, generalUpdateInp_t *generalUpdateInp ) {
    int status;
    status = procApiRequest( conn, GENERAL_UPDATE_AN, generalUpdateInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
