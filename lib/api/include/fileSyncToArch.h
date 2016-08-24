#ifndef FILE_SYNC_TO_ARCH_H__
#define FILE_SYNC_TO_ARCH_H__

#include "rcConnect.h"
#include "fileStageToCache.h"


#ifdef __cplusplus
extern "C"
#endif
int rcFileSyncToArch( rcComm_t *conn, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );

#endif
