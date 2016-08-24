#ifndef RS_DATA_OBJ_WRITE_HPP
#define RS_DATA_OBJ_WRITE_HPP

#include "rodsConnect.h"
#include "dataObjInpOut.h"
#include "rodsDef.h"

int rsDataObjWrite( rsComm_t *rsComm, openedDataObjInp_t *dataObjWriteInp, bytesBuf_t *dataObjWriteInpBBuf );
int l3Write( rsComm_t *rsComm, int l1descInx, int len, bytesBuf_t *dataObjWriteInpBBuf );
int _l3Write( rsComm_t *rsComm, int l3descInx, void *buf, int len );

#endif
