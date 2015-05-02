/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "dataPut.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataPut( rcComm_t *conn, dataOprInp_t *dataPutInp, portalOprOut_t **portalOprOut )
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
 * \param[in] dataPutInp
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
rcDataPut( rcComm_t *conn, dataOprInp_t *dataPutInp,
           portalOprOut_t **portalOprOut ) {
    int status;
    status = procApiRequest( conn, DATA_PUT_AN,  dataPutInp, NULL,
                             ( void ** ) portalOprOut, NULL );

    return status;
}
