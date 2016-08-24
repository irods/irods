#ifndef RS_SYNC_MOUNTED_COLL_HPP
#define RS_SYNC_MOUNTED_COLL_HPP

#include "objInfo.h"
#include "rcConnect.h"
#include "dataObjInpOut.h"


int rsSyncMountedColl( rsComm_t *rsComm, dataObjInp_t *syncMountedCollInp );
int _rsSyncMountedColl( rsComm_t *rsComm, specColl_t *specColl, int oprType );

#endif
