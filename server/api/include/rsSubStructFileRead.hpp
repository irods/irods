#ifndef RS_SUB_STRUCT_FILE_READ_HPP
#define RS_SUB_STRUCT_FILE_READ_HPP

#include "rodsConnect.h"
#include "subStructFileRead.h"

int rsSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp, bytesBuf_t *subStructFileReadOutBBuf );
int _rsSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp, bytesBuf_t *subStructFileReadOutBBuf );
int remoteSubStructFileRead( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReadInp, bytesBuf_t *subStructFileReadOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
