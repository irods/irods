#ifndef SUB_STRUCT_FILE_GET_H__
#define SUB_STRUCT_FILE_GET_H__

#include "irods/rcConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsDef.h"

int rcSubStructFileGet( rcComm_t *conn, subFile_t *subFile, bytesBuf_t *subFileGetOutBBuf );

#endif
