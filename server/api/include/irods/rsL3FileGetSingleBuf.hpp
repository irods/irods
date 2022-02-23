#ifndef RS_L3_FILE_GET_SINGLE_BUF_HPP
#define RS_L3_FILE_GET_SINGLE_BUF_HPP

#include "irods/rodsDef.h"
#include "irods/rcConnect.h"

int rsL3FileGetSingleBuf( rsComm_t *rsComm, int *l1descInx, bytesBuf_t *dataObjOutBBuf );

#endif
