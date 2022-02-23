#ifndef RS_SUB_STRUCT_FILE_PUT_HPP
#define RS_SUB_STRUCT_FILE_PUT_HPP

#include "irods/rodsConnect.h"
#include "irods/objInfo.h"
#include "irods/rodsDef.h"

int rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );
int _rsSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf );
int remoteSubStructFilePut( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFilePutOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
