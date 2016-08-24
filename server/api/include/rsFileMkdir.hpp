#ifndef RS_FILE_MKDIR_HPP
#define RS_FILE_MKDIR_HPP

#include "rodsConnect.h"
#include "rodsDef.h"

int rsFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp );
int _rsFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp );
int remoteFileMkdir( rsComm_t *rsComm, fileMkdirInp_t *fileMkdirInp, rodsServerHost_t *rodsServerHost );

#endif
