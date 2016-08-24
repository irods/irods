#ifndef SUB_STRUCT_FILE_PUT_H__
#define SUB_STRUCT_FILE_PUT_H__

#include "rcConnect.h"
#include "objInfo.h"
#include "rodsDef.h"


int rcSubStructFilePut( rcComm_t *conn, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );

#endif
