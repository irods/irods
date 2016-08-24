#ifndef RS_DATA_OBJ_RSYNC_HPP
#define RS_DATA_OBJ_RSYNC_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"
#include "msParam.h"

int rsDataObjRsync( rsComm_t *rsComm, dataObjInp_t *dataObjInp, msParamArray_t **outParamArray );
int rsRsyncDataToFile( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int rsRsyncFileToData( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int rsRsyncDataToData( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int _rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjInp, char **outChksumStr, dataObjInfo_t **dataObjInfoHead );

#endif
