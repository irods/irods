#ifndef RS_DATA_OBJ_READ_HPP
#define RS_DATA_OBJ_READ_HPP

#include "irods/rodsDef.h"
#include "irods/dataObjInpOut.h"
#include "irods/rcConnect.h"
#include "irods/fileRead.h"

int rsDataObjRead( rsComm_t *rsComm, openedDataObjInp_t *dataObjReadInp, bytesBuf_t *dataObjReadOutBBuf );
int l3Read( rsComm_t *rsComm, int l1descInx, int len, bytesBuf_t *dataObjReadOutBBuf );
int _l3Read( rsComm_t *rsComm, int l3descInx, void *buf, int len );

#endif
