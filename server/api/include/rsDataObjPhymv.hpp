#ifndef RS_DATA_OBJ_PHYMV_HPP
#define RS_DATA_OBJ_PHYMV_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjPhymv( rsComm_t *rsComm, dataObjInp_t *dataObjInp, transferStat_t **transferStat );
int _rsDataObjPhymv( rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *srcDataObjInfoHead, const char *resc_name, transferStat_t *transStat, int multiCopyFlag );

#endif
