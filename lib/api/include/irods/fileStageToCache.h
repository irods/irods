#ifndef IRODS_FILE_STAGE_TO_CACHE_H
#define IRODS_FILE_STAGE_TO_CACHE_H

#include "irods/rodsType.h"
#include "irods/rodsDef.h"
#include "irods/objInfo.h"
#include "irods/rcConnect.h"

typedef struct FileStageSyncInp {
    int mode;
    int flags;
    rodsLong_t dataSize;
    rodsHostAddr_t addr;
    char filename[MAX_NAME_LEN];
    char cacheFilename[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    keyValPair_t condInput;
} fileStageSyncInp_t;
#define fileStageSyncInp_PI "int mode; int flags; double dataSize; struct RHostAddr_PI; str filename[MAX_NAME_LEN]; str cacheFilename[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; struct KeyValPair_PI;"

typedef struct FileSyncOut {
    char file_name[MAX_NAME_LEN];
} fileSyncOut_t;
#define fileSyncOut_PI "str file_name[MAX_NAME_LEN];"


#ifdef __cplusplus
extern "C"
#endif
int rcFileStageToCache( rcComm_t *conn, fileStageSyncInp_t *fileStageToCacheInp );

#endif // IRODS_FILE_STAGE_TO_CACHE_H
