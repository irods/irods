#ifndef RS_SUB_STRUCT_FILE_GET_HPP
#define RS_SUB_STRUCT_FILE_GET_HPP

#include "objInfo.h"
#include "rodsDef.h"
#include "rodsConnect.h"

int rsSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFileGetOutBBuf );
int _rsSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFileGetOutBBuf );
int remoteSubStructFileGet( rsComm_t *rsComm, subFile_t *subFile, bytesBuf_t *subFileGetOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
