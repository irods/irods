#ifndef RS_DATA_OBJ_UNLINK_HPP
#define RS_DATA_OBJ_UNLINK_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjUnlink( rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp );
int dataObjUnlinkS( rsComm_t *rsComm, dataObjInp_t *dataObjUnlinkInp, dataObjInfo_t *dataObjInfo );
int l3Unlink( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );

#endif
