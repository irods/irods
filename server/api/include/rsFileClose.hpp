#ifndef RS_FILE_CLOSE_HPP
#define RS_FILE_CLOSE_HPP

#include "rodsConnect.h"
#include "fileClose.h"

int rsFileClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp );
int _rsFileClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp );
int remoteFileClose( rsComm_t *rsComm, fileCloseInp_t *fileCloseInp, rodsServerHost_t *rodsServerHost );

#endif
