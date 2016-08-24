#ifndef SYNC_MOUNTED_COLL_H__
#define SYNC_MOUNTED_COLL_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"

// flag for oprType of dataObjInp_t and structFileOprInp_t.
#define PURGE_STRUCT_FILE_CACHE 0x1
#define DELETE_STRUCT_FILE      0x2
#define NO_REG_COLL_INFO        0x4     // don't register collInfo even if struct file is changed
#define LOGICAL_BUNDLE          0x8     // use the filePath associated with the logical path instead of the path in cacheDir
#define CREATE_TAR_OPR          0x0     // create tar file -c
#define ADD_TO_TAR_OPR          0x10    // add to a tar file
#define PRESERVE_COLL_PATH      0x20    // preserver the last entry of coll
#define PRESERVE_DIR_CONT       0x40    // preserve the content of cachrdir

#ifdef __cplusplus
extern "C"
#endif
int rcSyncMountedColl( rcComm_t *conn, dataObjInp_t *syncMountedCollInp );

#endif
