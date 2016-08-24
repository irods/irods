#ifndef RS_STREAM_READ_HPP
#define RS_STREAM_READ_HPP

#include "rodsConnect.h"
#include "objInfo.h"
#include "fileRead.h"

int rsStreamRead( rsComm_t *rsComm, fileReadInp_t *streamReadInp, bytesBuf_t *streamReadOutBBuf );

#endif
