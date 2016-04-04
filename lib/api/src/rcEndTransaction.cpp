#include "endTransaction.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcEndTransaction( rcComm_t *conn, endTransactionInp_t *endTransactionInp )
 *
 * \brief End a database transaction.
 *
 * \user client
 *
 * \ingroup database
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in]  endTransactionInp - The transaction to be ended.
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
**/
int
rcEndTransaction( rcComm_t *conn, endTransactionInp_t *endTransactionInp ) {
    int status;
    status = procApiRequest( conn, END_TRANSACTION_AN,  endTransactionInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
