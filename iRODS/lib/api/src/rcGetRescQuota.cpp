#include "getRescQuota.h"
#include "procApiRequest.h"
#include "apiNumber.h"

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
rcGetRescQuota( rcComm_t *conn, getRescQuotaInp_t *getRescQuotaInp,
                rescQuota_t **rescQuota ) {
    int status;
    status = procApiRequest( conn, GET_RESC_QUOTA_AN,  getRescQuotaInp, NULL,
                             ( void ** ) rescQuota, NULL );

    return status;
}
