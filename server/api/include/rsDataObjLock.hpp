#ifndef RS_DATA_OBJ_LOCK_HPP
#define RS_DATA_OBJ_LOCK_HPP

#include "rcConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

int rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );

#endif
