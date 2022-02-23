#ifndef STREAM_READ_H__
#define STREAM_READ_H__

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/fileRead.h"


#ifdef __cplusplus
extern "C"
#endif
int rcStreamRead( rcComm_t *conn, fileReadInp_t *streamReadInp, bytesBuf_t *streamReadOutBBuf );

#endif
