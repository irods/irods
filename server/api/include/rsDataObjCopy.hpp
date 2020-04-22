#ifndef RS_DATA_OBJ_COPY_HPP
#define RS_DATA_OBJ_COPY_HPP

#include "objInfo.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"
#include "dataObjCopy.h"

int rsDataObjCopy( rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp, transferStat_t **transferStat );
int getAndConnRemoteZoneForCopy( rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp, rodsServerHost_t **rodsServerHost );

#endif
