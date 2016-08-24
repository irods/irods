#ifndef RS_FILE_GET_HPP
#define RS_FILE_GET_HPP

#include "rodsConnect.h"
#include "rodsDef.h"
#include "fileOpen.h"

int rsFileGet( rsComm_t *rsComm, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf );
int _rsFileGet( rsComm_t *rsComm, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf );
int remoteFileGet( rsComm_t *rsComm, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf, rodsServerHost_t *rodsServerHost );

#endif
