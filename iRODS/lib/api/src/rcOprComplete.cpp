#include "oprComplete.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcOprComplete( rcComm_t *conn, int retval )
 *
 * \brief Complete an operation.
 *
 * \user client
 *
 * \ingroup miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[out] retval
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcOprComplete( rcComm_t *conn, int retval ) {
    int status;
    status = procApiRequest( conn, OPR_COMPLETE_AN, ( void ** )( static_cast< void * >( &retval ) ), NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
