/* This is script-generated code.  */ 
/* See getRescQuota.h for a description of this API call.*/

#include "getRescQuota.h"

int
rcGetRescQuota (rcComm_t *conn, getRescQuotaInp_t *getRescQuotaInp,
rescQuota_t **rescQuota)
{
    int status;
    status = procApiRequest (conn, GET_RESC_QUOTA_AN,  getRescQuotaInp, NULL, 
        (void **) rescQuota, NULL);

    return (status);
}
