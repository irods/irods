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
    PathCacheTable *pctable = ( PathCacheTable * ) malloc( sizeof( PathCacheTable ) );
    pctable->PathCacheLock = &pctable->lock;
    pctable->NonExistPathTable = newHashTable( NUM_PATH_HASH_SLOT );
    pctable->PathArrayTable = newHashTable( NUM_PATH_HASH_SLOT );
    pthread_mutex_init( pctable->PathCacheLock, NULL ); // *&
    return pctable;
}

pathCache_t *
matchPathCache( PathCacheTable *pctable, const char *inPath ) {
    LOCK( *pctable->PathCacheLock );
    pathCache_t * pathCache = ( pathCache_t * )lookupFromHashTable( pctable->PathArrayTable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return pathCache;
}

int
addPathToCache( char *inPath, fileCache_t *fileCache, Hashtable *pathQueArray, struct stat *stbuf, pathCache_t **outPathCache ) {
    pathCache_t *tmpPathCache = newPathCache( inPath, fileCache, stbuf, time( 0 ) );
    insertIntoHashTable( pathQueArray, inPath, tmpPathCache );
    if ( outPathCache != NULL ) {
        *outPathCache = tmpPathCache;
    }
    return 0;
}

int
rmPathFromCache( char *inPath, Hashtable *pathQueArray, pthread_mutex_t *lock ) {
    LOCK( *lock );
    pathCache_t *tmpPathCache = ( pathCache_t * ) deleteFromHashTable( pathQueArray, inPath );
    UNLOCK( *lock );

    if ( tmpPathCache != NULL ) {
        _freePathCache( tmpPathCache );
        return 1;
    }
    else {
        return 0;
    }
}

int updatePathCacheStatFromFileCache( pathCache_t *tmpPathCache ) {

    LOCK_STRUCT( *tmpPathCache );
    tmpPathCache->stbuf.st_size = tmpPathCache->fileCache->fileSize;
    if ( tmpPathCache->fileCache->state != NO_FILE_CACHE ) {
        struct stat stbuf;
        if ( stat( tmpPathCache->fileCache->fileCachePath, &stbuf ) < 0 ) {
            int errsav = errno;
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
            UNLOCK_STRUCT( *tmpPathCache );
            return errsav ? ( -1 * errsav ) : -1;
        }
        /* update the size */
        tmpPathCache->stbuf.st_uid = stbuf.st_uid;
        tmpPathCache->stbuf.st_gid = stbuf.st_gid;
        /* tmpPathCache->stbuf.st_atim = stbuf.st_atime;
        tmpPathCache->stbuf.st_ctim = stbuf.st_ctime; */
        tmpPathCache->stbuf.st_mtime = stbuf.st_mtime;
        tmpPathCache->stbuf.st_nlink = stbuf.st_nlink;
        UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
    }
    UNLOCK_STRUCT( *tmpPathCache );
    return 0;
}

int pathNotExist( PathCacheTable *pctable, char *path ) {
    rmPathFromCache( ( char * ) path, pctable->PathArrayTable, pctable->PathCacheLock );
    rmPathFromCache( ( char * ) path, pctable->NonExistPathTable, pctable->PathCacheLock );

    LOCK( *pctable->PathCacheLock );
    insertIntoHashTable( pctable->NonExistPathTable, ( char * ) path, NULL );
    UNLOCK( *pctable->PathCacheLock );
    return 0;
}

int pathExist( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache ) {
    rmPathFromCache( ( char * ) inPath, pctable->PathArrayTable, pctable->PathCacheLock );
    rmPathFromCache( ( char * ) inPath, pctable->NonExistPathTable, pctable->PathCacheLock );
    addPathToCache( inPath, fileCache, pctable->PathArrayTable, stbuf, outPathCache );
    return 0;
}

int pathReplace( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache ) {
    rmPathFromCache( inPath, pctable->PathArrayTable, pctable->PathCacheLock );
    addPathToCache( inPath, fileCache, pctable->PathArrayTable, stbuf, outPathCache );

    LOCK( *pctable->PathCacheLock );
    deleteFromHashTable( pctable->NonExistPathTable, ( char * ) inPath );
    UNLOCK( *pctable->PathCacheLock );

    return 0;
}

int lookupPathExist( PathCacheTable *pctable, char *inPath, pathCache_t **paca ) {
    LOCK( *pctable->PathCacheLock );
    *paca = ( pathCache_t * ) lookupFromHashTable( pctable->NonExistPathTable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return *paca == NULL ? 0 : 1;
}

int lookupPathNotExist( PathCacheTable *pctable, char *inPath ) {
    LOCK( *pctable->PathCacheLock );
    pathCache_t* paca = ( pathCache_t * ) lookupFromHashTable( pctable->NonExistPathTable, inPath );
    UNLOCK( *pctable->PathCacheLock );
    return paca == NULL ? 0 : 1;
}

int clearPathFromCache( PathCacheTable *pctable, char *inPath ) {
    rmPathFromCache( inPath, pctable->PathArrayTable, pctable->PathCacheLock );

    LOCK( *pctable->PathCacheLock );
    deleteFromHashTable( pctable->NonExistPathTable, inPath );
    UNLOCK( *pctable->PathCacheLock );

    return 0;
}

int addFileCacheForPath( pathCache_t *pathCache, fileCache_t *fileCache ) {
    if ( pathCache->fileCache != NULL ) {
        /* todo close fileCache if necessary */
        UNREF( pathCache->fileCache, FileCache );
    }
    REF( pathCache->fileCache, fileCache );
    return 0;
}

