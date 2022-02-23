#ifndef RS_REG_DATA_OBJ_HPP
#define RS_REG_DATA_OBJ_HPP

#include "irods/rcConnect.h"
#include "irods/objInfo.h"

int rsRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, dataObjInfo_t **outDataObjInfo );
int _rsRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );
int svrRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo );

#endif
