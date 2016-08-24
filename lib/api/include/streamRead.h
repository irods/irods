#ifndef STREAM_READ_H__
#define STREAM_READ_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "fileRead.h"


#ifdef __cplusplus
extern "C"
#endif
int rcStreamRead( rcComm_t *conn, fileReadInp_t *streamReadInp, bytesBuf_t *streamReadOutBBuf );

#endif
