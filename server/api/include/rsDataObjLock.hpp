#ifndef RS_DATA_OBJ_LOCK_HPP
#define RS_DATA_OBJ_LOCK_HPP

#include "rcConnect.h"
#include "objInfo.h"
#include "dataObjInpOut.h"

int rsDataObjLock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjLock( dataObjInp_t *dataObjInp );
int getLockCmdAndType( keyValPair_t *condInput, int *cmd, int *type );
int rsDataObjUnlock( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjUnlock( dataObjInp_t *dataObjInp );

#endif
