#ifndef RS_L3_FILE_PUT_SINGLE_BUF_HPP
#define RS_L3_FILE_PUT_SINGLE_BUF_HPP

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"

int rsL3FilePutSingleBuf( rsComm_t *rsComm, int *l1descInx, bytesBuf_t *dataObjInBBuf );
int l3FilePutSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjInpBBuf );

#endif
