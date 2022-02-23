#ifndef RS_DATA_OBJ_PUT_HPP
#define RS_DATA_OBJ_PUT_HPP

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"
#include "irods/procApiRequest.h"
#include "irods/dataObjInpOut.h"

int rsDataObjPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut );
int preProcParaPut( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut );

#endif
