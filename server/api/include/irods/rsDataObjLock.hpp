#ifndef RS_DATA_OBJ_LOCK_HPP
#define RS_DATA_OBJ_LOCK_HPP

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"

int rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );

#endif
