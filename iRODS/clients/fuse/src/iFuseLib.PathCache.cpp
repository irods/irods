/*** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.hpp"
#include "iFuseLib.hpp"
#include "iFuseOper.hpp"
#include "hashtable.hpp"
#include "restructs.hpp"
#include "iFuseLib.Lock.hpp"

PathCacheTable *initPathCache() {
	PathCacheTable *pctable = (PathCacheTable *) malloc(sizeof (PathCacheTable));
	pctable->PathCacheLock = &pctable->lock;
    pctable->NonExistPathTable = newHashTable( NUM_PATH_HASH_SLOT );
    pctable->PathArrayTable = newHashTable( NUM_PATH_HASH_SLOT );
    pthread_mutex_init( pctable->PathCacheLock, NULL ); // *&
    return pctable;
}

int
matchAndLockPathCache( PathCacheTable *pctable, char *inPath, pathCache_t **outPathCache ) {
    int status;
    LOCK( *pctable->PathCacheLock );
    status = _matchAndLockPathCache( pctable, inPath, outPathCache );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}

int
_matchAndLockPathCache( PathCacheTable *pctable, char *inPath, pathCache_t **outPathCache ) {
    *outPathCache = ( pathCache_t * )lookupFromHashTable( pctable->PathArrayTable, inPath );

    if ( *outPathCache != NULL ) {
        LOCK_STRUCT( **outPathCache );
        return 1;
    }

    return 0;
}

int
addPathToCache( pthread_mutex_t *PathCacheLock, char *inPath, fileCache_t *fileCache, Hashtable *pathQueArray, struct stat *stbuf, pathCache_t **outPathCache ) {
    int status;

    LOCK( *PathCacheLock );
    status = _addPathToCache( inPath, fileCache, pathQueArray, stbuf, outPathCache );
    UNLOCK( *PathCacheLock );
    return status;
}

int
_addPathToCache( char *inPath, fileCache_t *fileCache, Hashtable *pathQueArray, struct stat *stbuf, pathCache_t **outPathCache ) {
    pathCache_t *tmpPathCache = newPathCache( inPath, fileCache, stbuf, time( 0 ) );
    insertIntoHashTable( pathQueArray, inPath, tmpPathCache );
    if ( outPathCache != NULL ) {
        *outPathCache = tmpPathCache;
    }
    return 0;
}

int
rmPathFromCache( pthread_mutex_t *PathCacheLock, char *inPath, Hashtable *pathQueArray ) {
    int status;
    LOCK( *PathCacheLock );
    status = _rmPathFromCache( inPath, pathQueArray );
    UNLOCK( *PathCacheLock );
    return status;
}

int
_rmPathFromCache( char *inPath, Hashtable *pathQueArray ) {
    pathCache_t *tmpPathCache = ( pathCache_t * ) deleteFromHashTable( pathQueArray, inPath );

    if ( tmpPathCache != NULL ) {
        _freePathCache( tmpPathCache );
        return 1;
    }
    else {
        return 0;
    }
}

int updatePathCacheStatFromFileCache( pathCache_t *tmpPathCache ) {
    int status;
    LOCK_STRUCT( *tmpPathCache );
    status = _updatePathCacheStatFromFileCache( tmpPathCache );
    UNLOCK_STRUCT( *tmpPathCache );
    return status;

}
int _updatePathCacheStatFromFileCache( pathCache_t *tmpPathCache ) {
    int status;

    tmpPathCache->stbuf.st_size = tmpPathCache->fileCache->fileSize;
    if ( tmpPathCache->fileCache->state != NO_FILE_CACHE ) {
        struct stat stbuf;
        status = stat( tmpPathCache->fileCache->fileCachePath, &stbuf );
        if ( status < 0 ) {
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
            return errno ? ( -1 * errno ) : -1;
        }
        else {
            /* update the size */
            tmpPathCache->stbuf.st_uid = stbuf.st_uid;
            tmpPathCache->stbuf.st_gid = stbuf.st_gid;
            /* tmpPathCache->stbuf.st_atim = stbuf.st_atime;
            tmpPathCache->stbuf.st_ctim = stbuf.st_ctime; */
            tmpPathCache->stbuf.st_mtime = stbuf.st_mtime;
            tmpPathCache->stbuf.st_nlink = stbuf.st_nlink;
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
            return 0;
        }
    }
    else {
        return 0;
    }
}

int _pathNotExist( PathCacheTable *pctable, char *path ) {
    _rmPathFromCache( ( char * ) path, pctable->PathArrayTable );
    _rmPathFromCache( ( char * ) path, pctable->NonExistPathTable );

    insertIntoHashTable( pctable->NonExistPathTable, ( char * ) path, NULL );
    return 0;
}

int _pathExist( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache ) {
    _rmPathFromCache( ( char * ) inPath, pctable->PathArrayTable );
    _rmPathFromCache( ( char * ) inPath, pctable->NonExistPathTable );
    _addPathToCache( inPath, fileCache, pctable->PathArrayTable, stbuf, outPathCache );
    return 0;
}

int _pathReplace( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache ) {
    _rmPathFromCache( inPath, pctable->PathArrayTable );
    _addPathToCache( inPath, fileCache, pctable->PathArrayTable, stbuf, outPathCache );
    deleteFromHashTable( pctable->NonExistPathTable, ( char * ) inPath );
    return 0;
}

int _lookupPathExist( PathCacheTable *pctable, char *inPath, pathCache_t **paca ) {
    return ( *paca = ( pathCache_t * ) lookupFromHashTable( pctable->NonExistPathTable, inPath ) ) == NULL ? 0 : 1;
}

int lookupPathExist( PathCacheTable *pctable, char *inPath, pathCache_t **paca ) {
    int status;
    LOCK( *pctable->PathCacheLock );
    status = _lookupPathExist( pctable, inPath, paca );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}
int _lookupPathNotExist( PathCacheTable *pctable, char *inPath ) {
    return lookupFromHashTable( pctable->NonExistPathTable, inPath ) == NULL ? 0 : 1;
}

int lookupPathNotExist( PathCacheTable *pctable, char *inPath ) {
    int status;
    LOCK( *pctable->PathCacheLock );
    status = _lookupPathNotExist( pctable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}
int pathNotExist( PathCacheTable *pctable, char *inPath ) {
    int status = 0;
    LOCK( *pctable->PathCacheLock );
    status = _pathNotExist( pctable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}
int pathExist( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache ) {
    int status = 0;
    LOCK( *pctable->PathCacheLock );
    status = _pathExist( pctable, inPath, fileCache, stbuf, outPathCache );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}

int _clearPathFromCache( PathCacheTable *pctable, char *inPath ) {
    _rmPathFromCache( inPath, pctable->PathArrayTable );
    deleteFromHashTable( pctable->NonExistPathTable, inPath );
    return 0;
}

int clearPathFromCache( PathCacheTable *pctable, char *inPath ) {
    int status = 0;
    LOCK( *pctable->PathCacheLock );
    status = _clearPathFromCache( pctable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return status;
}


int _addFileCacheForPath( pathCache_t *pathCache, fileCache_t *fileCache ) {
    if ( pathCache->fileCache != NULL ) {
        /* todo close fileCache if necessary */
        UNREF( pathCache->fileCache, FileCache );
    }
    REF( pathCache->fileCache, fileCache );
    return 0;
}

