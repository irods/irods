#ifndef L3_FILE_GET_SINGLE_BUF_H__
#define L3_FILE_GET_SINGLE_BUF_H__

#include "rodsDef.h"
#include "rcConnect.h"

#ifdef __cplusplus
extern "C"
#endif
int rcL3FileGetSingleBuf( rcComm_t *conn, int l1descInx, bytesBuf_t *dataObjOutBBuf );

#endif
