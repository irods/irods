#ifndef RS_DATA_OBJ_GET_HPP
#define RS_DATA_OBJ_GET_HPP

#include "rodsConnect.h"
#include "rodsDef.h"
#include "dataObjInpOut.h"

int rsDataObjGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp, portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf );
int _rsDataObjGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp, portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf, int handlerFlag, dataObjInfo_t *dataObjInfoHead );
int preProcParaGet( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut );
int l3DataGetSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjOutBBuf, portalOprOut_t **portalOprOut );
int l3FileGetSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjOutBBuf );

#endif
