#include "irods/get_logical_quota.h"

#include "irods/apiNumber.h"
#include "irods/procApiRequest.h"

/**
 * \fn rc_get_logical_quota( struct RcComm *conn, getLogicalQuotaInp_t *getLogicalQuotaInp, logicalQuotaList_t **logicalQuotaList )
 *
 * \brief Gets the logical quota(s) that apply to a collection.
 *
 * \user client
 *
 * \ingroup server_miscellaneous
 *
 * \since 1.0
 *
 * \remark none
 *
 * \note The input structure may contain a null collection name. In that case, if the connection is privileged (i.e. an admin) every logical quota will be fetched.
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] getLogicalQuotaInp
 * \param[out] logicalQuotaList - the list of applicable quotas
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rc_get_logical_quota( struct RcComm *conn, getLogicalQuotaInp_t *getLogicalQuotaInp,
                logicalQuotaList_t **logicalQuotaList ) {
    return procApiRequest(conn, GET_LOGICAL_QUOTA_AN,  getLogicalQuotaInp, nullptr, reinterpret_cast<void**>(logicalQuotaList), nullptr);

}
