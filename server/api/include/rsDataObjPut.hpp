#ifndef RS_DATA_OBJ_PUT_HPP
#define RS_DATA_OBJ_PUT_HPP

#include "rcConnect.h"
#include "rodsDef.h"
#include "procApiRequest.h"
#include "dataObjInpOut.h"

int rsDataObjPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut );
int _rsDataObjPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut );
int preProcParaPut( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut );
int l3DataPutSingleBuf( rsComm_t *rsComm, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf );
int _l3DataPutSingleBuf( rsComm_t *rsComm, int l1descInx, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf );
int l3FilePutSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjInpBBuf );

#endif
