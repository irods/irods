#include "unregDataObj.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcUnregDataObj( rcComm_t *conn, unregDataObj_t *unregDataObjInp )
 *
 * \brief Unregister a data object.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] unregDataObjInp - the dataObj to unregister
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcUnregDataObj( rcComm_t *conn, unregDataObj_t *unregDataObjInp ) {
    int status;
    status = procApiRequest( conn, UNREG_DATA_OBJ_AN, unregDataObjInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}
