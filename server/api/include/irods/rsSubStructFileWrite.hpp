#ifndef RS_SUB_STRUCT_FILE_WRITE_HPP
#define RS_SUB_STRUCT_FILE_WRITE_HPP

#include "irods/rodsConnect.h"
#include "irods/rodsDef.h"
#include "irods/subStructFileRead.h"

int rsSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp, bytesBuf_t *subStructFileWriteOutBBuf );
int _rsSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp, bytesBuf_t *subStructFileWriteOutBBuf );
int remoteSubStructFileWrite( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileWriteInp, bytesBuf_t *subStructFileWriteOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
