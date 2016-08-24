#ifndef RS_GET_RESC_QUOTA_HPP
#define RS_GET_RESC_QUOTA_HPP

#include "rcConnect.h"
#include "rodsType.h"
#include "rodsGenQuery.h"
#include "getRescQuota.h"

int rsGetRescQuota( rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp, rescQuota_t **rescQuota );
int _rsGetRescQuota( rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp, rescQuota_t **rescQuota );
int getQuotaByResc( rsComm_t *rsComm, char *userName, char *rescName, genQueryOut_t **genQueryOut );
int queRescQuota( rescQuota_t **rescQuota, genQueryOut_t *genQueryOut );
int fillRescQuotaStruct( rescQuota_t *rescQuota, char *tmpQuotaLimit, char *tmpQuotaOver, char *tmpRescName, char *tmpQuotaRescId, char *tmpQuotaUserId );
int updatequotaOverrun( const char *_resc_hier, rodsLong_t dataSize, int flags );
int chkRescQuotaPolicy( rsComm_t *rsComm );
int setRescQuota(rsComm_t* comm_handle, const char* obj_path, const char* resc_name, rodsLong_t data_size);


#endif
