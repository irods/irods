/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* iFuseLib.c - The misc lib functions for the iRODS/Fuse server. 
 */



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.h"
#include "iFuseLib.h"
#include "iFuseOper.h"

#include <cstdlib>
#include <iostream>

#include <boost/thread/thread_time.hpp>
static boost::mutex DescLock;
static boost::mutex ConnLock;
static boost::mutex PathCacheLock;
static boost::mutex NewlyCreatedOprLock;
boost::thread*            ConnManagerThr;
boost::mutex              ConnManagerLock;
boost::condition_variable ConnManagerCond;

char FuseCacheDir[MAX_NAME_LEN];

/* some global variables */
iFuseDesc_t IFuseDesc[MAX_IFUSE_DESC];
int IFuseDescInuseCnt = 0;
iFuseConn_t *ConnHead = NULL;
connReqWait_t *ConnReqWaitQue = NULL;
rodsEnv MyRodsEnv;

static int ConnManagerStarted = 0;

pathCacheQue_t NonExistPathArray[NUM_PATH_HASH_SLOT];
pathCacheQue_t PathArray[NUM_PATH_HASH_SLOT];
newlyCreatedFile_t NewlyCreatedFile[NUM_NEWLY_CREATED_SLOT];
char *ReadCacheDir = NULL;

static specialPath_t SpecialPath[] = {
    {"/tls", 4},
    {"/i686", 5},
    {"/sse2", 5},
    {"/lib", 4},
    {"/librt.so.1", 11},
    {"/libacl.so.1", 12},
    {"/libselinux.so.1", 16},
    {"/libc.so.6", 10},
    {"/libpthread.so.0", 16},
    {"/libattr.so.1", 13},
};

static int NumSpecialPath = sizeof (SpecialPath) / sizeof (specialPath_t);

int
initPathCache ()
{
    bzero (NonExistPathArray, sizeof (NonExistPathArray));
    bzero (PathArray, sizeof (PathArray));
    bzero (NewlyCreatedFile, sizeof (NewlyCreatedFile));
    return (0);
}

int
isSpecialPath (char *inPath)
{
    int len;
    char *endPtr;
    int i;

    if (inPath == NULL) {
        rodsLog (LOG_ERROR,
          "isSpecialPath: input inPath is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    len = strlen (inPath);
    endPtr = inPath + len;
    for (i = 0; i < NumSpecialPath; i++) {
	if (len < SpecialPath[i].len) continue;
	if (strcmp (SpecialPath[i].path, endPtr - SpecialPath[i].len) == 0)
	    return (1);
    }
    return 0;
}

int
matchPathInPathCache (char *inPath, pathCacheQue_t *pathQueArray,
pathCache_t **outPathCache)
{
    int status;
    PathCacheLock.lock();
    status = _matchPathInPathCache (inPath, pathQueArray, outPathCache);
    PathCacheLock.unlock();
    return status;
}

int
_matchPathInPathCache (char *inPath, pathCacheQue_t *pathQueArray,
pathCache_t **outPathCache)
{
    int mysum, myslot;
    int status;
    pathCacheQue_t *myque;

    if (inPath == NULL) {
        rodsLog (LOG_ERROR,
          "matchPathInPathCache: input inPath is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    mysum = pathSum (inPath);
    myslot = getHashSlot (mysum, NUM_PATH_HASH_SLOT);
    myque = &pathQueArray[myslot];

    chkCacheExpire (myque);
    status = matchPathInPathSlot (myque, inPath, outPathCache);

    return status;
}

int
addPathToCache (char *inPath, pathCacheQue_t *pathQueArray,
struct stat *stbuf, pathCache_t **outPathCache)
{
    int status;

    PathCacheLock.lock();
    status = _addPathToCache (inPath, pathQueArray, stbuf, outPathCache);
    PathCacheLock.unlock();
    return status;
}

int
_addPathToCache (char *inPath, pathCacheQue_t *pathQueArray,
struct stat *stbuf, pathCache_t **outPathCache)
{
    pathCacheQue_t *pathCacheQue;
    int mysum, myslot;
    int status;

    /* XXXX if (isSpecialPath ((char *) inPath) != 1) return 0; */
    mysum = pathSum (inPath);
    myslot = getHashSlot (mysum, NUM_PATH_HASH_SLOT);
    pathCacheQue = &pathQueArray[myslot];
    status = addToCacheSlot (inPath, pathCacheQue, stbuf, outPathCache);

    return (status);
}

int
rmPathFromCache (char *inPath, pathCacheQue_t *pathQueArray)
{
    int status;
    PathCacheLock.lock();
    status = _rmPathFromCache (inPath, pathQueArray);
    PathCacheLock.unlock();
    return status;
}

int
_rmPathFromCache (char *inPath, pathCacheQue_t *pathQueArray)
{
    pathCacheQue_t *pathCacheQue;
    int mysum, myslot;
    pathCache_t *tmpPathCache;

    /* XXXX if (isSpecialPath ((char *) inPath) != 1) return 0; */
    mysum = pathSum (inPath);
    myslot = getHashSlot (mysum, NUM_PATH_HASH_SLOT);
    pathCacheQue = &pathQueArray[myslot];

    tmpPathCache = pathCacheQue->top;
    while (tmpPathCache != NULL) {
        if (strcmp (tmpPathCache->filePath, inPath) == 0) {
            if (tmpPathCache->prev == NULL) {
                /* top */
                pathCacheQue->top = tmpPathCache->next;
            } else {
                tmpPathCache->prev->next = tmpPathCache->next;
            }
            if (tmpPathCache->next == NULL) {
		/* bottom */
		pathCacheQue->bottom = tmpPathCache->prev;
	    } else {
		tmpPathCache->next->prev = tmpPathCache->prev;
	    }
	    freePathCache (tmpPathCache);
	    return 1;
	}
        tmpPathCache = tmpPathCache->next;
    }
    return 0;
}

int
getHashSlot (int value, int numHashSlot)
{
    int mySlot = value % numHashSlot;

    return (mySlot);
}

int
matchPathInPathSlot (pathCacheQue_t *pathCacheQue, char *inPath, 
pathCache_t **outPathCache)
{
    pathCache_t *tmpPathCache;

    *outPathCache = NULL;
    if (pathCacheQue == NULL) {
        rodsLog (LOG_ERROR,
          "matchPathInPathSlot: input pathCacheQue is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    tmpPathCache = pathCacheQue->top;
    while (tmpPathCache != NULL) {
	if (strcmp (tmpPathCache->filePath, inPath) == 0) {
	    *outPathCache = tmpPathCache;
	    return 1;
	}
	tmpPathCache = tmpPathCache->next;
    }
    return (0);
}

int
chkCacheExpire (pathCacheQue_t *pathCacheQue)
{
    pathCache_t *tmpPathCache;

    uint curTime = time (0);
    if (pathCacheQue == NULL) {
        rodsLog (LOG_ERROR,
          "chkCacheExpire: input pathCacheQue is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    tmpPathCache = pathCacheQue->top;
    while (tmpPathCache != NULL) {
	if (curTime >= tmpPathCache->cachedTime + CACHE_EXPIRE_TIME) {
	    /* cache expired */
	    pathCacheQue->top = tmpPathCache->next;
	    freePathCache (tmpPathCache);
	    tmpPathCache = pathCacheQue->top;
            if (tmpPathCache != NULL) {
                tmpPathCache->prev = NULL;
            } else {
		pathCacheQue->bottom = NULL;
		return (0);
	    }
	} else {
	    /* not expired */
	    return (0);
	}
    }
    return (0);
}
	     
int
addToCacheSlot (char *inPath, pathCacheQue_t *pathCacheQue, 
struct stat *stbuf, pathCache_t **outPathCache)
{
    pathCache_t *tmpPathCache;
    
    if (pathCacheQue == NULL || inPath == NULL) {
        rodsLog (LOG_ERROR,
          "addToCacheSlot: input pathCacheQue or inPath is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    tmpPathCache = (pathCache_t *) malloc (sizeof (pathCache_t));
    if (outPathCache != NULL) *outPathCache = tmpPathCache;
    bzero (tmpPathCache, sizeof (pathCache_t));
    tmpPathCache->filePath = strdup (inPath);
    tmpPathCache->cachedTime = time (0);
    tmpPathCache->pathCacheQue = pathCacheQue;
    if (stbuf != NULL) {
	tmpPathCache->stbuf = *stbuf;
    }
    /* queue it to the bottom */
    if (pathCacheQue->top == NULL) {
	pathCacheQue->top = pathCacheQue->bottom = tmpPathCache;
    } else {
	pathCacheQue->bottom->next = tmpPathCache;
	tmpPathCache->prev = pathCacheQue->bottom;
	pathCacheQue->bottom = tmpPathCache;
    }
    return (0);
}

int
pathSum (char *inPath)
{
    int len, i;
    int mysum = 0;

    if (inPath == NULL) {
        rodsLog (LOG_ERROR,
          "pathSum: input inPath is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    len = strlen (inPath);

    for (i = 0; i < len; i++) {
	mysum += inPath[i];
    }

    return mysum; 
}

int
initIFuseDesc ()
{
    // JMC - overwrites objects construction? -  memset (IFuseDesc, 0, sizeof (iFuseDesc_t) * MAX_IFUSE_DESC);
    return (0);
}

int
allocIFuseDesc ()
{
    int i;

    DescLock.lock();
    for (i = 3; i < MAX_IFUSE_DESC; i++) {
        if (IFuseDesc[i].inuseFlag <= IRODS_FREE) {
	    IFuseDesc[i].mutex = new boost::mutex; // JMC :: necessary since no ctor/dtor on struct
            IFuseDesc[i].inuseFlag = IRODS_INUSE;
	    IFuseDescInuseCnt++;
            DescLock.unlock();
            return (i);
        };
    }
    DescLock.unlock();
    rodsLog (LOG_ERROR, 
      "allocIFuseDesc: Out of iFuseDesc");

    return (SYS_OUT_OF_FILE_DESC);
}

int 
lockDesc (int descInx)
{
    int status;

    if (descInx < 3 || descInx >= MAX_IFUSE_DESC) {
        rodsLog (LOG_ERROR,
         "lockDesc: descInx %d out of range", descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    try {
       IFuseDesc[descInx].mutex->lock();
    } catch( boost::thread_resource_error ) {
       status = -1;
    }
    return status;
}

int
unlockDesc (int descInx)
{
    int status;

    if (descInx < 3 || descInx >= MAX_IFUSE_DESC) {
        rodsLog (LOG_ERROR,
         "unlockDesc: descInx %d out of range", descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    try {
        IFuseDesc[descInx].mutex->unlock(); 
    } catch ( boost::thread_resource_error ) {
        status = -1;
    }
    return status;
}

int
iFuseConnInuse (iFuseConn_t *iFuseConn)
{
    int i;
    int inuseCnt = 0;

    if (iFuseConn == NULL) return 0;
    DescLock.lock();
    for (i = 3; i < MAX_IFUSE_DESC; i++) {
	if (inuseCnt >= IFuseDescInuseCnt) break;
        if (IFuseDesc[i].inuseFlag == IRODS_INUSE) {
	    inuseCnt++;
	    if (IFuseDesc[i].iFuseConn != NULL && 
	      IFuseDesc[i].iFuseConn == iFuseConn) {
                DescLock.unlock();
                return 1;
	    }
	}
    }
    DescLock.unlock();
    return (0);
}

int
freePathCache (pathCache_t *tmpPathCache)
{
    if (tmpPathCache == NULL) return 0;
    if (tmpPathCache->filePath != NULL) free (tmpPathCache->filePath);
    if (tmpPathCache->locCacheState != NO_FILE_CACHE &&
      tmpPathCache->locCachePath != NULL) {
	freeFileCache (tmpPathCache);
    }
    free (tmpPathCache);
    return (0);
}

int
freeFileCache (pathCache_t *tmpPathCache)
{
    unlink (tmpPathCache->locCachePath);
    free (tmpPathCache->locCachePath);
    tmpPathCache->locCachePath = NULL;
    tmpPathCache->locCacheState = NO_FILE_CACHE;
    return 0;
}

int
freeIFuseDesc (int descInx)
{
    int i;
    iFuseConn_t *tmpIFuseConn = NULL;

    if (descInx < 3 || descInx >= MAX_IFUSE_DESC) {
        rodsLog (LOG_ERROR,
         "freeIFuseDesc: descInx %d out of range", descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    DescLock.lock();
    for (i = 0; i < MAX_BUF_CACHE; i++) {
        if (IFuseDesc[descInx].bufCache[i].buf != NULL) {
	    free (IFuseDesc[descInx].bufCache[i].buf);
	}
    }
    if (IFuseDesc[descInx].objPath != NULL)
	free (IFuseDesc[descInx].objPath);

    if (IFuseDesc[descInx].localPath != NULL)
	free (IFuseDesc[descInx].localPath);
    delete IFuseDesc[descInx].mutex;
    IFuseDesc[descInx].mutex = 0;
    tmpIFuseConn = IFuseDesc[descInx].iFuseConn;
    if (tmpIFuseConn != NULL) {
	IFuseDesc[descInx].iFuseConn = NULL;
    }
    memset (&IFuseDesc[descInx], 0, sizeof (iFuseDesc_t));
    IFuseDescInuseCnt--;

    DescLock.unlock();
    /* have to do it outside the lock bacause _relIFuseConn lock it */
    if (tmpIFuseConn != NULL)
	_relIFuseConn (tmpIFuseConn);

    return (0);
}

int 
checkFuseDesc (int descInx)
{
    if (descInx < 3 || descInx >= MAX_IFUSE_DESC) {
        rodsLog (LOG_ERROR,
         "checkFuseDesc: descInx %d out of range", descInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }

    if (IFuseDesc[descInx].inuseFlag != IRODS_INUSE) {
        rodsLog (LOG_ERROR,
         "checkFuseDesc: descInx %d is not inuse", descInx);
        return (SYS_BAD_FILE_DESCRIPTOR);
    }
    if (IFuseDesc[descInx].iFd <= 0) {
        rodsLog (LOG_ERROR,
         "checkFuseDesc:  iFd %d of descInx %d <= 0", 
	  IFuseDesc[descInx].iFd, descInx);
        return (SYS_BAD_FILE_DESCRIPTOR);
    }

    return (0);
}

int
fillIFuseDesc (int descInx, iFuseConn_t *iFuseConn, int iFd, char *objPath,
char *localPath)
{ 
    IFuseDesc[descInx].iFuseConn = iFuseConn;
    IFuseDesc[descInx].iFd = iFd;
    if (objPath != NULL) {
        /* rstrcpy (IFuseDesc[descInx].objPath, objPath, MAX_NAME_LEN); */
        IFuseDesc[descInx].objPath = strdup (objPath);
    }
    if (localPath != NULL) {
        /* rstrcpy (IFuseDesc[descInx].localPath, localPath, MAX_NAME_LEN); */
        IFuseDesc[descInx].localPath = strdup (localPath);
    }
    return (0);
}

int
ifuseClose (char *path, int descInx)
{
    int lockFlag;
    int status;

    if (IFuseDesc[descInx].iFuseConn != NULL &&
      IFuseDesc[descInx].iFuseConn->conn != NULL) {
        useIFuseConn (IFuseDesc[descInx].iFuseConn);
	lockFlag = 1;
    } else {
	lockFlag = 0;
    }
    status = _ifuseClose (path, descInx);
    if (lockFlag == 1) {
	unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
    }
    return status;
}

int
_ifuseClose (char *path, int descInx)
{
    int status = 0;
    int savedStatus = 0;
    int goodStat = 0;

    if (IFuseDesc[descInx].locCacheState == NO_FILE_CACHE) {
	status = closeIrodsFd (IFuseDesc[descInx].iFuseConn->conn, 
	  IFuseDesc[descInx].iFd);
    } else {	/* cached */
        if (IFuseDesc[descInx].newFlag > 0 || 
	  IFuseDesc[descInx].locCacheState == HAVE_NEWLY_CREATED_CACHE) {
            pathCache_t *tmpPathCache;
            /* newly created. Just update the size */
            if (matchPathInPathCache ((char *) path, PathArray,
             &tmpPathCache) == 1 && tmpPathCache->locCachePath != NULL) {
                status = updatePathCacheStat (tmpPathCache);
                if (status >= 0) goodStat = 1;
		status = ifusePut (IFuseDesc[descInx].iFuseConn->conn, 
		  path, tmpPathCache->locCachePath,
		  IFuseDesc[descInx].createMode, 
		  tmpPathCache->stbuf.st_size);
                if (status < 0) {
                    rodsLog (LOG_ERROR,
                      "ifuseClose: ifusePut of %s error, status = %d",
                       path, status);
                    savedStatus = -EBADF;
                }
		if (tmpPathCache->stbuf.st_size > MAX_READ_CACHE_SIZE) {
		    /* too big to keep */
		    freeFileCache (tmpPathCache);
		}	
            } else {
                /* should not be here. but cache may be removed that we
		 * may have to deal with it */
                rodsLog (LOG_ERROR,
                  "ifuseClose: IFuseDesc indicated a newly created cache, but does not exist for %s",
                   path);
		savedStatus = -EBADF;
	    }
	}
	status = close (IFuseDesc[descInx].iFd);
	if (status < 0) {
	    status = (errno ? (-1 * errno) : -1);
	} else {
	    status = savedStatus;
	}
    }

    if (IFuseDesc[descInx].bytesWritten > 0 && goodStat == 0) 
        rmPathFromCache ((char *) path, PathArray);
    return (status);
}

int
ifusePut (rcComm_t *conn, char *path, char *locCachePath, int mode, 
rodsLong_t srcSize)
{
    dataObjInp_t dataObjInp;
    int status;

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    status = parseRodsPathStr ((char *) (path + 1) , &MyRodsEnv,
      dataObjInp.objPath);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "ifusePut: parseRodsPathStr of %s error", path);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    dataObjInp.dataSize = srcSize;
    dataObjInp.createMode = mode;
    dataObjInp.openFlags = O_RDWR;
    addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");
    addKeyVal (&dataObjInp.condInput, DATA_TYPE_KW, "generic");
    if (strlen (MyRodsEnv.rodsDefResource) > 0) {
        addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW,
          MyRodsEnv.rodsDefResource);
    }

    status = rcDataObjPut (conn, &dataObjInp, locCachePath);
    return (status);
}

int
ifuseWrite (char *path, int descInx, char *buf, size_t size,
off_t offset)
{
    int status, myError;
    char irodsPath[MAX_NAME_LEN];
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;


    bzero (&dataObjWriteInp, sizeof (dataObjWriteInp));
    if (IFuseDesc[descInx].locCacheState == NO_FILE_CACHE) {
        dataObjWriteInpBBuf.buf = (void *) buf;
        dataObjWriteInpBBuf.len = size;
        dataObjWriteInp.l1descInx = IFuseDesc[descInx].iFd;
        dataObjWriteInp.len = size;

        if (IFuseDesc[descInx].iFuseConn != NULL && 
	  IFuseDesc[descInx].iFuseConn->conn != NULL) {
	    useIFuseConn (IFuseDesc[descInx].iFuseConn);
            status = rcDataObjWrite (IFuseDesc[descInx].iFuseConn->conn, 
	      &dataObjWriteInp, &dataObjWriteInpBBuf);
	    unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
            if (status < 0) {
                if ((myError = getErrno (status)) > 0) {
                    return (-myError);
                } else {
                    return -ENOENT;
                }
            } else if (status != (int) size) {
                rodsLog (LOG_ERROR,
		  "ifuseWrite: IFuseDesc[descInx].conn for %s is NULL", path);
                return -ENOENT;
	    }
	} else {
            rodsLog (LOG_ERROR,
              "ifuseWrite: IFuseDesc[descInx].conn for %s is NULL", path);
            return -ENOENT;
        }
        IFuseDesc[descInx].offset += status;
    } else {
        status = write (IFuseDesc[descInx].iFd, buf, size);

        if (status < 0) return (errno ? (-1 * errno) : -1);
        IFuseDesc[descInx].offset += status;
	if (IFuseDesc[descInx].offset >= MAX_NEWLY_CREATED_CACHE_SIZE) {
	    int irodsFd; 
	    int status1;
	    struct stat stbuf;
	    char *mybuf;
	    rodsLong_t myoffset;

	    /* need to write it to iRODS */
	    if (IFuseDesc[descInx].iFuseConn != NULL &&
	      IFuseDesc[descInx].iFuseConn->conn != NULL) {
		useIFuseConn (IFuseDesc[descInx].iFuseConn);
	        irodsFd = dataObjCreateByFusePath (
		  IFuseDesc[descInx].iFuseConn->conn,
		  path, IFuseDesc[descInx].createMode, irodsPath);
		unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
	    } else {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: IFuseDesc[descInx].conn for %s is NULL", path);
		irodsFd = -ENOENT;
	    }
	    if (irodsFd < 0) {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: dataObjCreateByFusePath of %s error, stat=%d",
                 path, irodsFd);
		close (IFuseDesc[descInx].iFd);
		rmPathFromCache ((char *) path, PathArray);
                return -ENOENT;
	    }
	    status1 = fstat (IFuseDesc[descInx].iFd, &stbuf);
            if (status1 < 0) {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: fstat of %s error, errno=%d",
                 path, errno);
		close (IFuseDesc[descInx].iFd);
		rmPathFromCache ((char *) path, PathArray);
		return (errno ? (-1 * errno) : -1);
	    }
	    mybuf = (char *) malloc (stbuf.st_size);
	    lseek (IFuseDesc[descInx].iFd, 0, SEEK_SET);
	    status1 = read (IFuseDesc[descInx].iFd, mybuf, stbuf.st_size);
            if (status1 < 0) {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: read of %s error, errno=%d",
                 path, errno);
		close (IFuseDesc[descInx].iFd);
                rmPathFromCache ((char *) path, PathArray);
				free( mybuf ); // JMC cppcheck - leak
                return (errno ? (-1 * errno) : -1);
            }
            dataObjWriteInpBBuf.buf = (void *) mybuf;
            dataObjWriteInpBBuf.len = stbuf.st_size;
            dataObjWriteInp.l1descInx = irodsFd;
            dataObjWriteInp.len = stbuf.st_size;

	    if (IFuseDesc[descInx].iFuseConn != NULL &&
	      IFuseDesc[descInx].iFuseConn->conn != NULL) {
		useIFuseConn (IFuseDesc[descInx].iFuseConn);
                status1 = rcDataObjWrite (IFuseDesc[descInx].iFuseConn->conn, 
		  &dataObjWriteInp, &dataObjWriteInpBBuf);
		unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
	    } else {
                rodsLog (LOG_ERROR,
                 "ifuseWrite: IFuseDesc[descInx].conn for %s is NULL", path);
                status1 = -ENOENT;
            }
	    free (mybuf);
            close (IFuseDesc[descInx].iFd);
            rmPathFromCache ((char *) path, PathArray);

            if (status1 < 0) {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: rcDataObjWrite of %s error, status=%d",
                 path, status1);
                if ((myError = getErrno (status1)) > 0) {
                    status1 = (-myError);
                } else {
                    status1 = -ENOENT;
                }
		IFuseDesc[descInx].iFd = 0;
                return (status1);
	    } else {
		IFuseDesc[descInx].iFd = irodsFd;
		IFuseDesc[descInx].locCacheState = NO_FILE_CACHE;
		IFuseDesc[descInx].newFlag = 0;
	    }
	    /* one last thing - seek to the right offset */
            myoffset = IFuseDesc[descInx].offset;
	    IFuseDesc[descInx].offset = 0;
            if ((status1 = ifuseLseek ((char *) path, descInx, myoffset)) 
	      < 0) {
                rodsLog (LOG_ERROR,
                  "ifuseWrite: ifuseLseek of %s error, status=%d",
                 path, status1);
                if ((myError = getErrno (status1)) > 0) {
                    return (-myError);
                } else {
                    return -ENOENT;
                }
	    }
	}
    }
    IFuseDesc[descInx].bytesWritten += status;

    return status;
}

int
ifuseRead (char *path, int descInx, char *buf, size_t size, 
off_t offset)
{
    int status;

    if (IFuseDesc[descInx].locCacheState == NO_FILE_CACHE) {
        openedDataObjInp_t dataObjReadInp;
	bytesBuf_t dataObjReadOutBBuf;
	int myError;

        bzero (&dataObjReadInp, sizeof (dataObjReadInp));
        dataObjReadOutBBuf.buf = buf;
        dataObjReadOutBBuf.len = size;
        dataObjReadInp.l1descInx = IFuseDesc[descInx].iFd;
        dataObjReadInp.len = size;

	if (IFuseDesc[descInx].iFuseConn != NULL &&
	  IFuseDesc[descInx].iFuseConn->conn != NULL) {
	    useIFuseConn (IFuseDesc[descInx].iFuseConn);
            status = rcDataObjRead (IFuseDesc[descInx].iFuseConn->conn, 
	      &dataObjReadInp, &dataObjReadOutBBuf);
	    unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
            if (status < 0) {
                if ((myError = getErrno (status)) > 0) {
                    return (-myError);
                } else {
                    return -ENOENT;
                }
            }
	} else {
            rodsLog (LOG_ERROR,
              "ifusRead: IFuseDesc[descInx].conn for %s is NULL", path);
            status = -ENOENT;
        }
    } else {
	status = read (IFuseDesc[descInx].iFd, buf, size);

	if (status < 0) return (errno ? (-1 * errno) : -1);
    }
    IFuseDesc[descInx].offset += status;

    return status;
}

int
ifuseLseek (char *path, int descInx, off_t offset)
{
    int status;

    if (IFuseDesc[descInx].offset != offset) {
	if (IFuseDesc[descInx].locCacheState == NO_FILE_CACHE) {
            openedDataObjInp_t dataObjLseekInp;
            fileLseekOut_t *dataObjLseekOut = NULL;

	    bzero (&dataObjLseekInp, sizeof (dataObjLseekInp));
            dataObjLseekInp.l1descInx = IFuseDesc[descInx].iFd;
            dataObjLseekInp.offset = offset;
            dataObjLseekInp.whence = SEEK_SET;

	    if (IFuseDesc[descInx].iFuseConn != NULL &&
	      IFuseDesc[descInx].iFuseConn->conn != NULL) {
		useIFuseConn (IFuseDesc[descInx].iFuseConn);
                status = rcDataObjLseek (IFuseDesc[descInx].iFuseConn->conn, 
		  &dataObjLseekInp, &dataObjLseekOut);
		unuseIFuseConn (IFuseDesc[descInx].iFuseConn);
                if (dataObjLseekOut != NULL) free (dataObjLseekOut);
	    } else {
                rodsLog (LOG_ERROR,
                  "ifuseLseek: IFuseDesc[descInx].conn for %s is NULL", path);
                status = -ENOENT;
            }
	} else {
	    rodsLong_t lstatus;
	    lstatus = lseek (IFuseDesc[descInx].iFd, offset, SEEK_SET);
	    if (lstatus >= 0) {
		status = 0;
	    } else {
		status = lstatus;
	    }
	}

        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "ifuseLseek: lseek of %s error", path);
            return status;
        } else {
            IFuseDesc[descInx].offset = offset;
        }

    }
    return (0);
}

/* getIFuseConnByPath - try to use the same conn as opened desc of the
 * same path */
int
getIFuseConnByPath (iFuseConn_t **iFuseConn, char *localPath, 
rodsEnv *myRodsEnv)
{
    int i, status;
    int inuseCnt = 0;
    DescLock.lock();
    for (i = 3; i < MAX_IFUSE_DESC; i++) {
        if (inuseCnt >= IFuseDescInuseCnt) break;
        if (IFuseDesc[i].inuseFlag == IRODS_INUSE) {
	    inuseCnt++;
	    if (IFuseDesc[i].iFuseConn != NULL &&
              IFuseDesc[i].iFuseConn->conn != NULL &&
	      strcmp (localPath, IFuseDesc[i].localPath) == 0) {
		*iFuseConn = IFuseDesc[i].iFuseConn;
                ConnLock.lock();
                DescLock.unlock();
		_useIFuseConn (*iFuseConn);
		return 0;
	    }
	}
    }
    /* no match. just assign one */
    DescLock.unlock();
    status = getIFuseConn (iFuseConn, myRodsEnv);

    return status;
}

int
getIFuseConn (iFuseConn_t **iFuseConn, rodsEnv *myRodsEnv)
{
    int status;
    iFuseConn_t *tmpIFuseConn;
    int inuseCnt;

    *iFuseConn = NULL;

    while (*iFuseConn == NULL) {
        ConnLock.lock();
        /* get a free IFuseConn */

	inuseCnt = 0;
        tmpIFuseConn = ConnHead;
        while (tmpIFuseConn != NULL) {
	    if (tmpIFuseConn->status == IRODS_FREE && 
	      tmpIFuseConn->conn != NULL) {
	        useFreeIFuseConn (tmpIFuseConn);
	        *iFuseConn = tmpIFuseConn;
		return 0;;
	    }
	    inuseCnt++;
	    tmpIFuseConn = tmpIFuseConn->next;
        }
        if (inuseCnt >= MAX_NUM_CONN) {
	    connReqWait_t myConnReqWait, *tmpConnReqWait;
	    /* find one that is not in use */
            tmpIFuseConn = ConnHead;
            while (tmpIFuseConn != NULL) {
                if (tmpIFuseConn->inuseCnt == 0 && 
	          tmpIFuseConn->pendingCnt == 0 && tmpIFuseConn->conn != NULL) {
                    _useIFuseConn (tmpIFuseConn);
                    *iFuseConn = tmpIFuseConn;
                    return 0;
                }
                tmpIFuseConn = tmpIFuseConn->next;
            }
	    /* have to wait */
	    bzero (&myConnReqWait, sizeof (myConnReqWait));
            myConnReqWait.mutex = new boost::mutex;
	    /* queue it to the bottom */
	    if (ConnReqWaitQue == NULL) {
	        ConnReqWaitQue = &myConnReqWait;
	    } else {
	        tmpConnReqWait = ConnReqWaitQue;
	        while (tmpConnReqWait->next != NULL) {
		    tmpConnReqWait = tmpConnReqWait->next;
	        }
	        tmpConnReqWait->next = &myConnReqWait;
	    }
	    while (myConnReqWait.state == 0) {
                boost::system_time const tt=boost::get_system_time() + 
                  boost::posix_time::seconds(CONN_REQ_SLEEP_TIME);
                ConnLock.unlock();
                boost::unique_lock< boost::mutex > 
		  boost_lock (*myConnReqWait.mutex);
                myConnReqWait.cond.timed_wait (boost_lock, tt);
	    }
            delete myConnReqWait.mutex;
	    /* start from begining */
	    continue;
        }

        ConnLock.unlock();
        /* get here when nothing free. make one */
        tmpIFuseConn = (iFuseConn_t *) malloc (sizeof (iFuseConn_t));
        if (tmpIFuseConn == NULL) {
            return SYS_MALLOC_ERR;
        }
        bzero (tmpIFuseConn, sizeof (iFuseConn_t));


        tmpIFuseConn->mutex = new boost::mutex;

        status = ifuseConnect (tmpIFuseConn, myRodsEnv);
        if (status < 0) return status;

        useIFuseConn (tmpIFuseConn);

        *iFuseConn = tmpIFuseConn;
        ConnLock.lock();
        /* queue it on top */
        tmpIFuseConn->next = ConnHead;
        ConnHead = tmpIFuseConn;

        ConnLock.unlock();
	break;
    }	/* while *iFuseConn */

    if (ConnManagerStarted < HIGH_NUM_CONN && 
      ++ConnManagerStarted == HIGH_NUM_CONN) {
	/* don't do it the first time */

	ConnManagerThr = new boost::thread( connManager );
        if (status < 0) {
            rodsLog (LOG_ERROR, "pthread_create failure, status = %d", status);
	    ConnManagerStarted --;	/* try again */
	}
    }
    return 0;
}

int
useIFuseConn (iFuseConn_t *iFuseConn)
{
    int status;
    if (iFuseConn == NULL || iFuseConn->conn == NULL)
        return USER__NULL_INPUT_ERR;
    ConnLock.lock();
    status = _useIFuseConn (iFuseConn);
    return status;
}

/* queIFuseConn - lock ConnLock before calling */ 
int
_useIFuseConn (iFuseConn_t *iFuseConn)
{
    if (iFuseConn == NULL || iFuseConn->conn == NULL) 
	return USER__NULL_INPUT_ERR;
    iFuseConn->actTime = time (NULL);
    iFuseConn->status = IRODS_INUSE;
    iFuseConn->pendingCnt++;

    ConnLock.unlock();
    iFuseConn->mutex->lock();
    ConnLock.lock();

    iFuseConn->inuseCnt++;
    iFuseConn->pendingCnt--;

    ConnLock.unlock();
    return 0;
}

int
useFreeIFuseConn (iFuseConn_t *iFuseConn)
{
    if (iFuseConn == NULL) return USER__NULL_INPUT_ERR;
    iFuseConn->actTime = time (NULL);
    iFuseConn->status = IRODS_INUSE;
    iFuseConn->inuseCnt++;
    ConnLock.unlock();
    iFuseConn->mutex->lock();
    return 0;
}

int
unuseIFuseConn (iFuseConn_t *iFuseConn)
{
    if (iFuseConn == NULL || iFuseConn->conn == NULL)
        return USER__NULL_INPUT_ERR;
    ConnLock.lock();
    iFuseConn->actTime = time (NULL);
    iFuseConn->inuseCnt--;
    ConnLock.unlock();
    iFuseConn->mutex->unlock();
    return 0;
}

int
ifuseConnect (iFuseConn_t *iFuseConn, rodsEnv *myRodsEnv)
{ 
    int status;
    rErrMsg_t errMsg;

    iFuseConn->conn = rcConnect (myRodsEnv->rodsHost, myRodsEnv->rodsPort,
      myRodsEnv->rodsUserName, myRodsEnv->rodsZone, NO_RECONN, &errMsg);

    if (iFuseConn->conn == NULL) {
	/* try one more */
        iFuseConn->conn = rcConnect (myRodsEnv->rodsHost, myRodsEnv->rodsPort,
          myRodsEnv->rodsUserName, myRodsEnv->rodsZone, NO_RECONN, &errMsg);
	if (iFuseConn->conn == NULL) {
            rodsLogError (LOG_ERROR, errMsg.status,
              "ifuseConnect: rcConnect failure %s", errMsg.msg);
            if (errMsg.status < 0) {
                return (errMsg.status);
            } else {
                return (-1);
	    }
        }
    }

    status = clientLogin (iFuseConn->conn);
    if (status != 0) {
        rcDisconnect (iFuseConn->conn);
		iFuseConn->conn=NULL; // JMC - backport 4589
    }
    return (status);
}

int
relIFuseConn (iFuseConn_t *iFuseConn)
{
    int status;

    if (iFuseConn == NULL) return USER__NULL_INPUT_ERR;
    unuseIFuseConn (iFuseConn);
    status = _relIFuseConn (iFuseConn);
    return status;
}
 
/* _relIFuseConn - call when calling from freeIFuseDesc where unuseIFuseConn
 * is not called */
int
_relIFuseConn (iFuseConn_t *iFuseConn)
{
    if (iFuseConn == NULL) return USER__NULL_INPUT_ERR;
    ConnLock.lock(); 
    iFuseConn->actTime = time (NULL);
    if (iFuseConn->conn == NULL) {
        /* unlock it before calling iFuseConnInuse which locks DescLock */
        ConnLock.unlock(); 
        if (iFuseConnInuse (iFuseConn) == 0) {
            ConnLock.lock();
            iFuseConn->status = IRODS_FREE;
            ConnLock.unlock();
        }
    } else if (iFuseConn->pendingCnt + iFuseConn->inuseCnt <= 0) {
        /* unlock it before calling iFuseConnInuse which locks DescLock */
        ConnLock.unlock(); 
        if (iFuseConnInuse (iFuseConn) == 0) {
            ConnLock.lock();
            iFuseConn->status = IRODS_FREE;
            ConnLock.unlock();
	}
    } else {
        ConnLock.unlock();
    }
    signalConnManager ();
    return 0;
}

int
signalConnManager ()
{
    int connCnt;
    ConnLock.lock();
    connCnt = getNumConn ();
    ConnLock.unlock();
    if (connCnt > HIGH_NUM_CONN) {
	ConnManagerCond.notify_all( );
    }

    return 0;
}

int
getNumConn ()
{
    iFuseConn_t *tmpIFuseConn;
    int connCnt = 0;
    tmpIFuseConn = ConnHead;
    while (tmpIFuseConn != NULL) {
	connCnt++;
	tmpIFuseConn = tmpIFuseConn->next;
    }
    return connCnt;
}

int
disconnectAll ()
{
    iFuseConn_t *tmpIFuseConn;
    ConnLock.lock();
    tmpIFuseConn = ConnHead;
    while (tmpIFuseConn != NULL) {
	if (tmpIFuseConn->conn != NULL) {
	    rcDisconnect (tmpIFuseConn->conn);
		tmpIFuseConn->conn=NULL; // JMC - backport 4589
	}
	tmpIFuseConn = tmpIFuseConn->next;
    }
    return 0;
}

void
connManager ()
{
    time_t curTime;
    iFuseConn_t *tmpIFuseConn, *savedIFuseConn;
    iFuseConn_t *prevIFuseConn;
    int freeCnt = 0;

    while (1) {
	int connCnt;
        curTime = time (NULL);

	ConnLock.lock();
        tmpIFuseConn = ConnHead;
	connCnt = 0;
	prevIFuseConn = NULL;
        while (tmpIFuseConn != NULL) {
	    if (tmpIFuseConn->status == IRODS_FREE) freeCnt ++;
	    if (curTime - tmpIFuseConn->actTime > IFUSE_CONN_TIMEOUT) {
		if (tmpIFuseConn->status == IRODS_FREE) {
		    /* can be disconnected */
		    if (tmpIFuseConn->conn != NULL) {
		        rcDisconnect (tmpIFuseConn->conn);
		        tmpIFuseConn->conn=NULL; // JMC - backport 4589
		    }
		    /* don't unlock. it will cause delete to fail */
		    delete tmpIFuseConn->mutex;
		    if (prevIFuseConn == NULL) {
			/* top */
			ConnHead = tmpIFuseConn->next;
		    } else {
			prevIFuseConn->next = tmpIFuseConn->next;
		    }
		    savedIFuseConn = tmpIFuseConn;
		    tmpIFuseConn = tmpIFuseConn->next;
		    free (savedIFuseConn);
		    continue;
		} 
	    }
	    connCnt++;
	    prevIFuseConn = tmpIFuseConn;
	    tmpIFuseConn = tmpIFuseConn->next;
	}
	if (MAX_NUM_CONN - connCnt > freeCnt) 
            freeCnt = MAX_NUM_CONN - connCnt;

	/* exceed high water mark for number of connection ? */
	if (connCnt > HIGH_NUM_CONN) {
            tmpIFuseConn = ConnHead;
            prevIFuseConn = NULL;
            while (tmpIFuseConn != NULL && connCnt > HIGH_NUM_CONN) {
		if (tmpIFuseConn->status == IRODS_FREE) {
                    /* can be disconnected */
                    if (tmpIFuseConn->conn != NULL) {
                        rcDisconnect (tmpIFuseConn->conn);
		tmpIFuseConn->conn=NULL; // JMC - backport 4589
                    }
		    /* don't unlock. it will cause delete to fail */
		    delete tmpIFuseConn->mutex;
                    if (prevIFuseConn == NULL) {
                        /* top */
                        ConnHead = tmpIFuseConn->next;
                    } else {
                        prevIFuseConn->next = tmpIFuseConn->next;
                    }
                    savedIFuseConn = tmpIFuseConn;
                    tmpIFuseConn = tmpIFuseConn->next;
                    free (savedIFuseConn);
		    connCnt--;
                    continue;
                }
                prevIFuseConn = tmpIFuseConn;
                tmpIFuseConn = tmpIFuseConn->next;
            }
	}
	/* wake up the ConnReqWaitQue if freeCnt > 0 */
        while (freeCnt > 0  && ConnReqWaitQue != NULL) {
            /* signal one in the wait queue */
            connReqWait_t *myConnReqWait;
            myConnReqWait = ConnReqWaitQue;
	    myConnReqWait->state = 1;
            ConnReqWaitQue = myConnReqWait->next;
            ConnLock.unlock();
            myConnReqWait->cond.notify_all( );
            ConnLock.lock();
            freeCnt--;
        }
        ConnLock.unlock();
	    boost::system_time const tt=boost::get_system_time() + boost::posix_time::seconds( CONN_MANAGER_SLEEP_TIME );
        boost::unique_lock< boost::mutex > boost_lock( ConnManagerLock );
        ConnManagerCond.timed_wait( boost_lock, tt );
    }
}

/* have to do this after getIFuseConn - lock */
int
ifuseReconnect (iFuseConn_t *iFuseConn)
{
    int status = 0;

    if (iFuseConn == NULL || iFuseConn->conn == NULL) 
	return USER__NULL_INPUT_ERR;
    rodsLog (LOG_DEBUG, "ifuseReconnect: reconnecting");
    rcDisconnect (iFuseConn->conn);
    status = ifuseConnect (iFuseConn, &MyRodsEnv);
    return status;
}

int
addNewlyCreatedToCache (char *path, int descInx, int mode, 
pathCache_t **tmpPathCache)
{
    int i;
    int newlyInx = -1;
    uint cachedTime = time (0);
    NewlyCreatedOprLock.lock();
    for (i = 0; i < NUM_NEWLY_CREATED_SLOT; i++) {
	if (newlyInx < 0 && NewlyCreatedFile[i].inuseFlag == IRODS_FREE) { 
	    newlyInx = i;
	    NewlyCreatedFile[i].inuseFlag = IRODS_INUSE;
	} else if (NewlyCreatedFile[i].inuseFlag == IRODS_INUSE) {
	    if (cachedTime - NewlyCreatedFile[i].cachedTime  >= 
	      MAX_NEWLY_CREATED_TIME) {
		closeNewlyCreatedCache (&NewlyCreatedFile[i]);
	        if (newlyInx < 0) {
		    newlyInx = i;
		} else {
		    NewlyCreatedFile[i].inuseFlag = IRODS_FREE;
		}
	    }
	}
    }
    if (newlyInx < 0) {
	/* have to close one */
	newlyInx = NUM_NEWLY_CREATED_SLOT - 2;
        closeNewlyCreatedCache (&NewlyCreatedFile[newlyInx]);
	NewlyCreatedFile[newlyInx].inuseFlag = IRODS_INUSE;
    }
    rstrcpy (NewlyCreatedFile[newlyInx].filePath, path, MAX_NAME_LEN);
    NewlyCreatedFile[newlyInx].descInx = descInx;
    NewlyCreatedFile[newlyInx].cachedTime = cachedTime;
    IFuseDesc[descInx].newFlag = 1;    /* XXXXXXX use newlyInx ? */
    fillFileStat (&NewlyCreatedFile[newlyInx].stbuf, mode, 0, cachedTime, 
      cachedTime, cachedTime);
    addPathToCache (path, PathArray, &NewlyCreatedFile[newlyInx].stbuf, 
      tmpPathCache);
    NewlyCreatedOprLock.unlock();
    return (0);
}

int
closeIrodsFd (rcComm_t *conn, int fd)
{
    int status;

    openedDataObjInp_t dataObjCloseInp;

    bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = fd;
    status = rcDataObjClose (conn, &dataObjCloseInp);
    return (status);
}

int
closeNewlyCreatedCache (newlyCreatedFile_t *newlyCreatedFile)
{
    int status = 0;

    if (newlyCreatedFile == NULL) return USER__NULL_INPUT_ERR;
    if (strlen (newlyCreatedFile->filePath) > 0) {
	int descInx = newlyCreatedFile->descInx;
	
	/* should not call irodsRelease because it will call
	 * getIFuseConn which will result in deadlock 
	 * irodsRelease (newlyCreatedFile->filePath, &fi); */
        if (checkFuseDesc (descInx) < 0) return -EBADF;
        status = ifuseClose ((char *) newlyCreatedFile->filePath, descInx);
        freeIFuseDesc (descInx);
	bzero (newlyCreatedFile, sizeof (newlyCreatedFile_t));
    }
    return (status);
}

int
getDescInxInNewlyCreatedCache (char *path, int flags)
{
    int descInx = -1;
    int i;
    NewlyCreatedOprLock.lock();
    for (i = 0; i < NUM_NEWLY_CREATED_SLOT; i++) {
        if (strcmp (path, NewlyCreatedFile[i].filePath) == 0) {
	    if ((flags & O_RDWR) == 0 && (flags & O_WRONLY) == 0) {
	        closeNewlyCreatedCache (&NewlyCreatedFile[i]);
	        descInx = -1;
	    } else if (checkFuseDesc (NewlyCreatedFile[i].descInx) >= 0) {
	        descInx = NewlyCreatedFile[i].descInx;
	        bzero (&NewlyCreatedFile[i], sizeof (newlyCreatedFile_t));
	    } else {
	        bzero (&NewlyCreatedFile[i], sizeof (newlyCreatedFile_t));
	        descInx = -1;
	    }
	    break;
	}
    }
    NewlyCreatedOprLock.unlock();
    return descInx;
}

int
fillFileStat (struct stat *stbuf, uint mode, rodsLong_t size, uint ctime,
uint mtime, uint atime)
{
    if (mode >= 0100)
        stbuf->st_mode = S_IFREG | mode;
    else
        stbuf->st_mode = S_IFREG | DEF_FILE_MODE;
    stbuf->st_size = size;

    stbuf->st_blksize = FILE_BLOCK_SZ;
    stbuf->st_blocks = (stbuf->st_size / FILE_BLOCK_SZ) + 1;

    stbuf->st_nlink = 1;
    stbuf->st_ino = random ();
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

int
fillDirStat (struct stat *stbuf, uint ctime, uint mtime, uint atime)
{
    stbuf->st_mode = S_IFDIR | DEF_DIR_MODE;
    stbuf->st_size = DIR_SZ;

    stbuf->st_nlink = 2;
    stbuf->st_ino = random ();
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

int 
updatePathCacheStat (pathCache_t *tmpPathCache)
{
    int status;

    if (tmpPathCache->locCacheState != NO_FILE_CACHE &&
      tmpPathCache->locCachePath != NULL) {
	struct stat stbuf;
	status = stat (tmpPathCache->locCachePath, &stbuf);
	if (status < 0) {
	    return (errno ? (-1 * errno) : -1);
	} else {
	    /* update the size */
	    tmpPathCache->stbuf.st_size = stbuf.st_size; 
	    return 0; 
	}
    } else {
	return 0;
    }
}

/* need to call getIFuseConn before calling irodsMknodWithCache */
int
irodsMknodWithCache (char *path, mode_t mode, char *cachePath)
{
    int status;
    int fd;

    if ((status = getFileCachePath (path, cachePath)) < 0)
        return status;

    /* fd = creat (cachePath, mode); WRONLY */
    fd = open (cachePath, O_CREAT|O_EXCL|O_RDWR, mode);
    if (fd < 0) {
        rodsLog (LOG_ERROR,
          "irodsMknodWithCache: local cache creat error for %s, errno = %d",
          cachePath, errno);
        return(errno ? (-1 * errno) : -1);
    } else {
        return fd;
    }
}

/* need to call getIFuseConn before calling irodsOpenWithReadCache */
int
irodsOpenWithReadCache (iFuseConn_t *iFuseConn, char *path, int flags)
{
    pathCache_t *tmpPathCache = NULL;
    struct stat stbuf;
    int status;
    dataObjInp_t dataObjInp;
    char cachePath[MAX_NAME_LEN];
    int fd, descInx;

    /* do only O_RDONLY (0) */
    if ((flags & (O_WRONLY | O_RDWR)) != 0) return -1;

    if (_irodsGetattr (iFuseConn, path, &stbuf, &tmpPathCache) < 0 ||
      tmpPathCache == NULL) return -1;

    /* too big to cache */
    if (stbuf.st_size > MAX_READ_CACHE_SIZE) return -1;	

    if (tmpPathCache->locCachePath == NULL) {

        rodsLog (LOG_DEBUG, "irodsOpenWithReadCache: caching %s", path);

        memset (&dataObjInp, 0, sizeof (dataObjInp));
        if ((status = getFileCachePath (path, cachePath)) < 0) 
	    return status;
        /* get the file to local cache */
        status = parseRodsPathStr ((char *) (path + 1) , &MyRodsEnv,
          dataObjInp.objPath);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "irodsOpenWithReadCache: parseRodsPathStr of %s error", path);
            /* use ENOTDIR for this type of error */
            return -ENOTDIR;
        }
        dataObjInp.openFlags = flags;
        dataObjInp.dataSize = stbuf.st_size;

        status = rcDataObjGet (iFuseConn->conn, &dataObjInp, cachePath);

        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "irodsOpenWithReadCache: rcDataObjGet of %s error", 
	      dataObjInp.objPath);

	    return status; 
	}
        tmpPathCache->locCachePath = strdup (cachePath);
        tmpPathCache->locCacheState = HAVE_READ_CACHE;
    } else {
        rodsLog (LOG_DEBUG, "irodsOpenWithReadCache: read cache match for %s",
          path);
    }

    fd = open (tmpPathCache->locCachePath, flags);
    if (fd < 0) {
        rodsLog (LOG_ERROR,
          "irodsOpenWithReadCache: local cache open error for %s, errno = %d",
          tmpPathCache->locCachePath, errno);
	return(errno ? (-1 * errno) : -1);
    }

    descInx = allocIFuseDesc ();
    if (descInx < 0) {
        rodsLogError (LOG_ERROR, descInx,
          "irodsOpenWithReadCache: allocIFuseDesc of %s error", path);
		close( fd ); // JMC cppcheck - resource
        return -ENOENT;
    }
    fillIFuseDesc (descInx, iFuseConn, fd, dataObjInp.objPath,
      (char *) path);
    IFuseDesc[descInx].locCacheState = HAVE_READ_CACHE;

    return descInx;
}

int
getFileCachePath (char *inPath, char *cacehPath)
{
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    struct stat statbuf;

    if (inPath == NULL || cacehPath == NULL) {
        rodsLog (LOG_ERROR,
          "getFileCachePath: input inPath or cacehPath is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    splitPathByKey (inPath, myDir, myFile, '/');

    while (1)
    {
        snprintf (cacehPath, MAX_NAME_LEN, "%s/%s.%d", FuseCacheDir,
          myFile, (int) random ());
        if (stat (cacehPath, &statbuf) < 0) break;
    }
    return 0;
}

int
setAndMkFileCacheDir ()
{
    char *tmpStr, *tmpDir;
    struct passwd *myPasswd;
    int status;

    myPasswd = getpwuid(getuid());

    if ((tmpStr = getenv ("FuseCacheDir")) != NULL && strlen (tmpStr) > 0) {
	tmpDir = tmpStr;
    } else {
	tmpDir = FUSE_CACHE_DIR;
    }

    snprintf (FuseCacheDir, MAX_NAME_LEN, "%s/%s.%d", tmpDir,
      myPasswd->pw_name, getpid());

    if ((status = mkdirR ("/", FuseCacheDir, DEF_DIR_MODE)) < 0) {
        rodsLog (LOG_ERROR,
          "setAndMkFileCacheDir: mkdirR of %s error. status = %d", 
	  FuseCacheDir, status);
    }

    return (status);

}

int
dataObjCreateByFusePath (rcComm_t *conn, char *path, int mode, 
char *outIrodsPath)
{
    dataObjInp_t dataObjInp;
    int status;

    memset (&dataObjInp, 0, sizeof (dataObjInp));
    status = parseRodsPathStr ((char *) (path + 1) , &MyRodsEnv,
      dataObjInp.objPath);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "dataObjCreateByFusePath: parseRodsPathStr of %s error", path);
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    if (strlen (MyRodsEnv.rodsDefResource) > 0) {
        addKeyVal (&dataObjInp.condInput, RESC_NAME_KW,
          MyRodsEnv.rodsDefResource);
    }

    addKeyVal (&dataObjInp.condInput, DATA_TYPE_KW, "generic");
    /* dataObjInp.createMode = DEF_FILE_CREATE_MODE; */
    dataObjInp.createMode = mode;
    dataObjInp.openFlags = O_RDWR;
    dataObjInp.dataSize = -1;

    status = rcDataObjCreate (conn, &dataObjInp);
    clearKeyVal (&dataObjInp.condInput);
    if (status >= 0 && outIrodsPath != NULL)
	rstrcpy (outIrodsPath, dataObjInp.objPath, MAX_NAME_LEN);

    return status;
}

int
getNewlyCreatedDescByPath (char *path)
{
    int i;
    int inuseCnt = 0;

    DescLock.lock();
    for (i = 3; i < MAX_IFUSE_DESC; i++) {
	if (inuseCnt >= IFuseDescInuseCnt) break;
        if (IFuseDesc[i].inuseFlag == IRODS_INUSE) { 
            inuseCnt++;
	    if (IFuseDesc[i].locCacheState != HAVE_NEWLY_CREATED_CACHE ||
	      IFuseDesc[i].localPath == NULL) {
	        continue;
	    }
	    if (strcmp (IFuseDesc[i].localPath, path) == 0) { 
                DescLock.unlock();
                return (i);
            }
	}
    }
    DescLock.unlock();
    return (-1);
}

int
renmeOpenedIFuseDesc (pathCache_t *fromPathCache, char *to)
{
	int descInx;
	int status = 0;
	pathCache_t *tmpPathCache = NULL;

	if( ( descInx = getNewlyCreatedDescByPath ((char *)fromPathCache->filePath)) >= 3 ) {
	
		rmPathFromCache( (char *) to, PathArray);
		rmPathFromCache( (char *) to, NonExistPathArray);
		addPathToCache(  (char *) to, PathArray, &fromPathCache->stbuf, &tmpPathCache);
		if( NULL == tmpPathCache ) {
			rodsLogError (LOG_ERROR, descInx, "renmeOpenedIFuseDesc: addPathToCache Failed for path [ %s ]", to);
			return -ENOTDIR;
		}
		
		tmpPathCache->locCachePath = fromPathCache->locCachePath;
		fromPathCache->locCachePath = NULL;
		tmpPathCache->locCacheState = HAVE_NEWLY_CREATED_CACHE;
		fromPathCache->locCacheState = NO_FILE_CACHE;
		
		if (IFuseDesc[descInx].objPath != NULL) 
			free (IFuseDesc[descInx].objPath);

		IFuseDesc[descInx].objPath = (char *) malloc (MAX_NAME_LEN);
		status = parseRodsPathStr ((char *) (to + 1) , &MyRodsEnv,
		IFuseDesc[descInx].objPath);
		
		if (status < 0) {
			rodsLogError (LOG_ERROR, status,"renmeOpenedIFuseDesc: parseRodsPathStr of %s error", to);
			return -ENOTDIR;
		}

		if (IFuseDesc[descInx].localPath != NULL) 
			free (IFuseDesc[descInx].localPath);
		IFuseDesc[descInx].localPath = strdup (to);
		return 0;
	} else {
		return -ENOTDIR;
	}
}

