#ifndef RS_DATA_OBJ_CHKSUM_HPP
#define RS_DATA_OBJ_CHKSUM_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

int rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjChksumInp, char **outChksum );
int _rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjInp, char **outChksumStr, dataObjInfo_t **dataObjInfoHead );
int dataObjChksumAndRegInfo( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char **outChksumStr );
int verifyDatObjChksum( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char **outChksumStr );

#endif
