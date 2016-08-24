#ifndef SUB_STRUCT_FILE_WRITE_H__
#define SUB_STRUCT_FILE_WRITE_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "subStructFileRead.h"

int rcSubStructFileWrite( rcComm_t *conn, subStructFileFdOprInp_t *subStructFileWriteInp, bytesBuf_t *subStructFileWriteOutBBuf );

#endif
