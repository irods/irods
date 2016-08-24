#ifndef RS_SUB_STRUCT_FILE_PUT_HPP
#define RS_SUB_STRUCT_FILE_PUT_HPP

#include "rodsConnect.h"
#include "objInfo.h"
#include "rodsDef.h"

int rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );
int _rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );
int remoteSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
