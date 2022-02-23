#ifndef FILE_GET_H__
#define FILE_GET_H__

#include "irods/rcConnect.h"
#include "irods/rodsDef.h"
#include "irods/fileOpen.h"


#ifdef __cplusplus
extern "C"
#endif
int rcFileGet( rcComm_t *conn, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf );

#endif
