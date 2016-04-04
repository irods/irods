/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "dataGet.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataGet( rcComm_t *conn, dataOprInp_t *dataGetInp, portalOprOut_t **portalOprOut )
 *
 * \brief Gets a dataObject on the server.
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
 * \param[in] dataGetInp
 * \param[out] portalOprOut
 *
 * \return integer
 * \retval 0 on success.
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcDataGet( rcComm_t *conn, dataOprInp_t *dataGetInp,
           portalOprOut_t **portalOprOut ) {
    int status;
    status = procApiRequest( conn, DATA_GET_AN, dataGetInp, NULL,
                             ( void ** ) portalOprOut, NULL );

    return status;
}
