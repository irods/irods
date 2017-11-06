#ifndef RS_FILE_WRITE_HPP
#define RS_FILE_WRITE_HPP

#include "rodsConnect.h"
#include "fileWrite.h"

int rsFileWrite( rsComm_t *rsComm, const fileWriteInp_t *fileWriteInp, const bytesBuf_t *fileWriteInpBBuf );
int _rsFileWrite( rsComm_t *rsComm, const fileWriteInp_t *fileWriteInp, const bytesBuf_t *fileWriteInpBBuf );
int remoteFileWrite( rsComm_t *rsComm, const fileWriteInp_t *fileWriteInp, const bytesBuf_t *fileWriteInpBBuf, rodsServerHost_t *rodsServerHost );

#endif
