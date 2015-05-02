#include "bulkDataObjReg.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcBulkDataObjReg( rcComm_t *conn, genQueryOut_t *bulkDataObjRegInp, genQueryOut_t **bulkDataObjRegOut )
 *
 * \brief Register a bulk data object.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] bulkDataObjRegInp
 * \param[out] bulkDataObjRegOut
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcBulkDataObjReg( rcComm_t *conn, genQueryOut_t *bulkDataObjRegInp,
                  genQueryOut_t **bulkDataObjRegOut ) {
    int status;
    status = procApiRequest( conn, BULK_DATA_OBJ_REG_AN, bulkDataObjRegInp,
                             NULL, ( void ** ) bulkDataObjRegOut, NULL );

    return status;
}
