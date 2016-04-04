#include "generalRowPurge.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGeneralRowPurge( rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp )
 *
 * \brief Purge rows from special purpose tables. Admin only.
 *
 * \user client (the API is restricted to admin users only)
 *
 * \ingroup administration
 *
 * \since .5
 *
 *
 * \remark  This client/server calls is used to purge rows from certain
 * special-purpose tables.  See the rs file for the current list.
 *  Admin only.  Also see generalRowInsert.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] generalRowPurgeInp
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/
int
rcGeneralRowPurge( rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp ) {
    int status;
    status = procApiRequest( conn, GENERAL_ROW_PURGE_AN,  generalRowPurgeInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
