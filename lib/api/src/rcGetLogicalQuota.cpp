#include "irods/apiNumber.h"
#include "irods/getLogicalQuota.h"
#include "irods/procApiRequest.h"

/**
 * \fn rcGetRescQuota( rcComm_t *conn, getRescQuotaInp_t *getRescQuotaInp, rescQuota_t **rescQuota )
 *
 * \brief Get the quota for a resource.
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
 * \param[in] getRescQuotaInp
 * \param[out] rescQuota - the quota
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcGetLogicalQuota( rcComm_t *conn, getLogicalQuotaInp_t *getLogicalQuotaInp,
                logicalQuotaList_t **logicalQuotaList ) {
    int status;
    status = procApiRequest( conn, GET_LOGICAL_QUOTA_AN,  getLogicalQuotaInp, NULL,
                             ( void ** ) logicalQuotaList, NULL);

    return status;
}
