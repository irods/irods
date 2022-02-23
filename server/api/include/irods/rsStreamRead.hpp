#ifndef RS_STREAM_READ_HPP
#define RS_STREAM_READ_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"
#include "irods/fileRead.h"

int rsStreamRead( rsComm_t *rsComm, fileReadInp_t *streamReadInp, bytesBuf_t *streamReadOutBBuf );

#endif
