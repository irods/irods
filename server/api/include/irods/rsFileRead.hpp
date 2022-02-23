#ifndef RS_FILE_READ_HPP
#define RS_FILE_READ_HPP

#include "irods/rodsConnect.h"
#include "irods/fileRead.h"

int rsFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf );
int _rsFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf );
int remoteFileRead( rsComm_t *rsComm, fileReadInp_t *fileReadInp, bytesBuf_t *fileReadOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
