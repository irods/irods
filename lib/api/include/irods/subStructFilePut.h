#ifndef SUB_STRUCT_FILE_PUT_H__
#define SUB_STRUCT_FILE_PUT_H__

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsDef.h"


int rcSubStructFilePut( rcComm_t *conn, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );

#endif
