/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* iFuseLib.h - Header for for iFuseLib.c */

#ifndef I_FUSE_LIB_H
#define I_FUSE_LIB_H

#include "rodsClient.h"
#include "rodsPath.h"

#define CACHE_FUSE_PATH         1
#ifdef CACHE_FUSE_PATH
#define CACHE_FILE_FOR_READ     1
#define CACHE_FILE_FOR_NEWLY_CREATED     1
#endif

#define MAX_BUF_CACHE   2
#define MAX_IFUSE_DESC   512
#define MAX_READ_CACHE_SIZE   (1024*1024)	/* 1 mb */
#define MAX_NEWLY_CREATED_CACHE_SIZE   (4*1024*1024)	/* 4 mb */
#define HIGH_NUM_CONN	5	/* high water mark */
#define MAX_NUM_CONN	10

#define NUM_NEWLY_CREATED_SLOT	5
#define MAX_NEWLY_CREATED_TIME	5	/* in sec */

#define FUSE_CACHE_DIR	"/tmp/fuseCache"

#define IRODS_FREE		0
#define IRODS_INUSE	1 


#ifdef USE_BOOST
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#endif
// =-=-=-=-=-=-=-

typedef struct BufCache {
    rodsLong_t beginOffset;
    rodsLong_t endOffset;
    void *buf;
} bufCache_t;

typedef enum { 
    NO_FILE_CACHE,
    HAVE_READ_CACHE,
    HAVE_NEWLY_CREATED_CACHE,
} readCacheState_t;

typedef struct ConnReqWait {
#ifdef USE_BOOST
    boost::mutex* mutex;
    boost::condition_variable cond;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    int state;
    struct ConnReqWait *next;
} connReqWait_t;

typedef struct IFuseDesc {
    iFuseConn_t *iFuseConn;    
    bufCache_t  bufCache[MAX_BUF_CACHE];
    int actCacheInx;    /* (cacheInx + 1) currently active. 0 means no cache */
    int inuseFlag;      /* 0 means not in use */
    int iFd;    /* irods client fd */
    int newFlag;
    int createMode;
    rodsLong_t offset;
    rodsLong_t bytesWritten;
    char *objPath;
    char *localPath;
    readCacheState_t locCacheState;
#ifdef USE_BOOST
    boost::mutex* mutex;
#else
    pthread_mutex_t lock;
#endif
} iFuseDesc_t;

#define NUM_PATH_HASH_SLOT	201
#define CACHE_EXPIRE_TIME	600	/* 10 minutes before expiration */

typedef struct PathCache {
    char* filePath;
    char* locCachePath;
    struct stat stbuf;
    uint cachedTime;
    struct PathCache *prev;
    struct PathCache *next;
    void *pathCacheQue;
    readCacheState_t locCacheState;
} pathCache_t;

typedef struct PathCacheQue {
    pathCache_t *top;
    pathCache_t *bottom;
} pathCacheQue_t;

typedef struct specialPath {
    char *path;
    int len;
} specialPath_t;

typedef struct newlyCreatedFile {
    int descInx;
    int inuseFlag;      /* 0 means not in use */
    char filePath[MAX_NAME_LEN];
    struct stat stbuf;
    uint cachedTime;
} newlyCreatedFile_t;

#ifdef  __cplusplus
extern "C" {
#endif

int
initIFuseDesc ();
int
allocIFuseDesc ();
int
lockDesc (int descInx);
int
unlockDesc (int descInx);
int
iFuseConnInuse (iFuseConn_t *iFuseConn);
int
freeIFuseDesc (int descInx);
iFuseConn_t *
getIFuseConnByRcConn (rcComm_t *conn);
int
getIFuseConnByPath (iFuseConn_t **iFuseConn, char *localPath,
rodsEnv *myRodsEnv);
int
fillIFuseDesc (int descInx, iFuseConn_t *iFuseConn, int iFd, char *objPath,
char *localPath);
int
ifuseWrite (char *path, int descInx, char *buf, size_t size,
off_t offset);
int
ifuseRead (char *path, int descInx, char *buf, size_t size, 
off_t offset);
int
ifuseLseek (char *path, int descInx, off_t offset);
int
getIFuseConn (iFuseConn_t **iFuseConn, rodsEnv *MyRodsEnv);
int
useIFuseConn (iFuseConn_t *iFuseConn);
int
useFreeIFuseConn (iFuseConn_t *iFuseConn);
int
_useIFuseConn (iFuseConn_t *iFuseConn);
int
unuseIFuseConn (iFuseConn_t *iFuseConn);
int 
useConn (rcComm_t *conn);
int
relIFuseConn (iFuseConn_t *iFuseConn);
int
_relIFuseConn (iFuseConn_t *iFuseConn);
void
connManager ();
int
disconnectAll ();
int
signalConnManager ();
int
getNumConn ();
int
iFuseDescInuse ();
int
checkFuseDesc (int descInx);
int
initPathCache ();
int
getHashSlot (int value, int numHashSlot);
int
matchPathInPathSlot (pathCacheQue_t *pathCacheQue, char *inPath,
pathCache_t **outPathCache);
int
chkCacheExpire (pathCacheQue_t *pathCacheQue);
int
addPathToCache (char *inPath, pathCacheQue_t *pathQueArray,
struct stat *stbuf, pathCache_t **outPathCache);
int
_addPathToCache (char *inPath, pathCacheQue_t *pathQueArray,
struct stat *stbuf, pathCache_t **outPathCache);
int
addToCacheSlot (char *inPath, pathCacheQue_t *pathCacheQue,
struct stat *stbuf, pathCache_t **outPathCache);
int
pathSum (char *inPath);
int
matchPathInPathCache (char *inPath, pathCacheQue_t *pathQueArray,
pathCache_t **outPathCache);
int
_matchPathInPathCache (char *inPath, pathCacheQue_t *pathQueArray,
pathCache_t **outPathCache);
int
isSpecialPath (char *inPath);
int
rmPathFromCache (char *inPath, pathCacheQue_t *pathQueArray);
int
_rmPathFromCache (char *inPath, pathCacheQue_t *pathQueArray);
int
addNewlyCreatedToCache (char *path, int descInx, int mode,
pathCache_t **tmpPathCache);
int
closeNewlyCreatedCache (newlyCreatedFile_t *newlyCreatedFile);
int
closeIrodsFd (rcComm_t *conn, int fd);
int
getDescInxInNewlyCreatedCache (char *path, int flags);
int
fillDirStat (struct stat *stbuf, uint ctime, uint mtime, uint atime);
int
fillFileStat (struct stat *stbuf, uint mode, rodsLong_t size, uint ctime,
uint mtime, uint atime);
int
irodsMknodWithCache (char *path, mode_t mode, char *cachePath);
int
irodsOpenWithReadCache (iFuseConn_t *iFuseConn, char *path, int flags);
int
freePathCache (pathCache_t *tmpPathCache);
int
getFileCachePath (char *inPath, char *cacehPath);
int
setAndMkFileCacheDir ();
int 
updatePathCacheStat (pathCache_t *tmpPathCache);
int
ifuseClose (char *path, int descInx);
int
_ifuseClose (char *path, int descInx);
int
dataObjCreateByFusePath (rcComm_t *conn, char *path, int mode, 
char *outIrodsPath);
int
ifusePut (rcComm_t *conn, char *path, char *locCachePath, int mode,
rodsLong_t srcSize);
int
freeFileCache (pathCache_t *tmpPathCache);
int
ifuseReconnect (iFuseConn_t *iFuseConn);
int
ifuseConnect (iFuseConn_t *iFuseConn, rodsEnv *myRodsEnv);
int
getNewlyCreatedDescByPath (char *path);
int
renmeOpenedIFuseDesc (pathCache_t *fromPathCache, char *to);
#ifdef  __cplusplus
}
#endif

#endif	/* I_FUSE_LIB_H */
