#include "getRemoteZoneResc.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcGetRemoteZoneResc( rcComm_t *conn, dataObjInp_t *dataObjInp, rodsHostAddr_t **rescAddr )
 *
 * \brief Get a resource from the remote zone.
 *
 * \user client
 *
 * \ingroup server_miscellaneous
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - generic dataObj input
 *      \n \b objPath - the path of the data object.
 *      \n \b condInput - condition input (optional).
 *      \n \b REMOTE_ZONE_OPR_KW - specifies the type of remote zone operation
 *              (REMOTE_CREATE or REMOTE_OPEN)
 *
 * \param[out] rescAddr - the address of the resource
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcGetRemoteZoneResc( rcComm_t *conn, dataObjInp_t *dataObjInp,
                     rodsHostAddr_t **rescAddr ) {
    int status;
    status = procApiRequest( conn, GET_REMOTE_ZONE_RESC_AN, dataObjInp, NULL,
                             ( void ** ) rescAddr, NULL );

    return status;
}


