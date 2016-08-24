#ifndef RS_FILE_SYNC_TO_ARCH_HPP
#define RS_FILE_SYNC_TO_ARCH_HPP

#include "rodsConnect.h"
#include "fileStageToCache.h"

int rsFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );
int rsFileSyncToArchByHost( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t**, rodsServerHost_t *rodsServerHost );
int _rsFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );
int remoteFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t**, rodsServerHost_t *rodsServerHost );

#endif
