#ifndef RS_FILE_STAGE_TO_CACHE_HPP
#define RS_FILE_STAGE_TO_CACHE_HPP

#include "rodsConnect.h"
#include "fileStageToCache.h"

int rsFileStageToCache( rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp );
int rsFileStageToCacheByHost( rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp, rodsServerHost_t *rodsServerHost );
int _rsFileStageToCache( rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp );
int remoteFileStageToCache( rsComm_t *rsComm, fileStageSyncInp_t *fileStageToCacheInp, rodsServerHost_t *rodsServerHost );

#endif
