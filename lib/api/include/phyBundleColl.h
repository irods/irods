#ifndef PHY_BUNDLE_COLL_H__
#define PHY_BUNDLE_COLL_H__

#include "rcConnect.h"
#include "rodsType.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "miscUtil.h"
#include "structFileExtAndReg.h"

#define BUNDLE_RESC       "bundleResc"
#define TAR_BUNDLE_TYPE   "tar bundle"
#define BUNDLE_STR        "bundle"
#define BUNDLE_RESC_CLASS "bundle"

#define MAX_BUNDLE_SIZE    4 // 4 G
#define MAX_SUB_FILE_CNT   5120

typedef struct BunReplCache {
    rodsLong_t  dataId;
    char objPath[MAX_NAME_LEN];      // optional for ADMIN_KW
    char chksumStr[NAME_LEN];
    int srcReplNum;
    struct BunReplCache *next;
} bunReplCache_t;

typedef struct BunReplCacheHeader {
    int numSubFiles;
    rodsLong_t totSubFileSize;
    bunReplCache_t *bunReplCacheHead;
} bunReplCacheHeader_t;

typedef struct CurSubFileCond {
    char collName[MAX_NAME_LEN];
    char dataName[MAX_NAME_LEN];
    rodsLong_t dataId;
    char subPhyPath[MAX_NAME_LEN];
    char cachePhyPath[MAX_NAME_LEN];
    int cacheReplNum;
    rodsLong_t subFileSize;
    int bundled;
} curSubFileCond_t;


#ifdef __cplusplus
extern "C"
#endif
int rcPhyBundleColl( rcComm_t *conn, structFileExtAndRegInp_t *phyBundleCollInp );

#endif
