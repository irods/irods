/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "dataCopy.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataCopy( rcComm_t *conn, dataCopyInp_t *dataCopyInp )
 *
 * \brief Copies data from source to target.
 *
 * \user client
 *
 * \ingroup server_datatransfer
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
*
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataCopyInp
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcDataCopy( rcComm_t *conn, dataCopyInp_t *dataCopyInp ) {
    int status;
    status = procApiRequest( conn, DATA_COPY_AN, dataCopyInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
