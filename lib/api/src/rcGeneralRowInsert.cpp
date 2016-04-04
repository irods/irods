#include "generalRowInsert.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGeneralRowInsert( rcComm_t *conn, generalRowInsertInp_t *generalRowInsertInp )
 *
 * \brief Insert rows into special purpose tables. Admin only.
 *
 * \user client (the API is restricted to admin users only)
 *
 * \ingroup administration
 *
 * \since .5
 *
 *
 * \remark This client/server call is used to insert rows into certain
 *  special-purpose tables.  See the rs file for the current list.
 *  Admin only.  Also see generalRowPurge.
 *
 * \note none
 *
 * \usage
 *
 * \param[in] conn - A rcComm_t connection handle to the server
 * \param[in] generalRowInsertInp
 * \return integer
 * \retval 0 on success
 *
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/
int
rcGeneralRowInsert( rcComm_t *conn, generalRowInsertInp_t *generalRowInsertInp ) {
    int status;
    status = procApiRequest( conn, GENERAL_ROW_INSERT_AN,  generalRowInsertInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
