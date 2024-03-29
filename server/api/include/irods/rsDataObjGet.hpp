#ifndef RS_DATA_OBJ_GET_HPP
#define RS_DATA_OBJ_GET_HPP

#include "irods/rodsConnect.h"
#include "irods/rodsDef.h"
#include "irods/dataObjInpOut.h"

int rsDataObjGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp, portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf );
int preProcParaGet( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut );

#endif
