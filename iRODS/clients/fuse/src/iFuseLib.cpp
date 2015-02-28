/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* iFuseLib.c - The misc lib functions for the iRODS/Fuse server.
 */



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.hpp"
#include "iFuseLib.hpp"
#include "iFuseOper.hpp"
#include "hashtable.hpp"
#include "list.hpp"



/* some global variables */

rodsEnv MyRodsEnv;
extern PathCacheTable *pctable;

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

static int NumSpecialPath = sizeof( SpecialPath ) / sizeof( specialPath_t );


int
isSpecialPath( char *inPath ) {
    int len;
    char *endPtr;
    int i;

    if ( inPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "isSpecialPath: input inPath is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    len = strlen( inPath );
    endPtr = inPath + len;
    for ( i = 0; i < NumSpecialPath; i++ ) {
        if ( len < SpecialPath[i].len ) {
            continue;
        }
        if ( strcmp( SpecialPath[i].path, endPtr - SpecialPath[i].len ) == 0 ) {
            return 1;
        }
    }
    return 0;
}







int
ifusePut( rcComm_t *conn, char *objPath, char *cachePath, int mode,
          rodsLong_t srcSize ) {
    dataObjInp_t dataObjInp;
    int status;

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, objPath, MAX_NAME_LEN );
    dataObjInp.dataSize = srcSize;
    dataObjInp.createMode = mode;
    dataObjInp.openFlags = O_RDWR;
    addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );
    addKeyVal( &dataObjInp.condInput, DATA_TYPE_KW, "generic" );
    if ( strlen( MyRodsEnv.rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjInp.condInput, DEST_RESC_NAME_KW,
                   MyRodsEnv.rodsDefResource );
    }

    status = rcDataObjPut( conn, &dataObjInp, cachePath );
    return status;
}








int
closeIrodsFd( rcComm_t *conn, int fd ) {
    int status;

    openedDataObjInp_t dataObjCloseInp;

    bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = fd;
    status = rcDataObjClose( conn, &dataObjCloseInp );
    return status;
}


int
fillFileStat( struct stat *stbuf, uint mode, rodsLong_t size, uint ctime,
              uint mtime, uint atime ) {
    if ( mode >= 0100 ) {
        stbuf->st_mode = S_IFREG | mode;
    }
    else {
        stbuf->st_mode = S_IFREG | DEF_FILE_MODE;
    }
    stbuf->st_size = size;

    stbuf->st_blksize = FILE_BLOCK_SZ;
    stbuf->st_blocks = ( stbuf->st_size / FILE_BLOCK_SZ ) + 1;

    stbuf->st_nlink = 1;
    stbuf->st_ino = random();
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

int
fillDirStat( struct stat *stbuf, uint ctime, uint mtime, uint atime ) {
    stbuf->st_mode = S_IFDIR | DEF_DIR_MODE;
    stbuf->st_size = DIR_SZ;

    stbuf->st_nlink = 2;
    stbuf->st_ino = random();
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

/* need to call getIFuseConn before calling irodsMknodWithCache */
int
irodsMknodWithCache( char *path, mode_t mode, char *cachePath ) {
    int status;
    int fd;

    if ( ( status = getFileCachePath( path, cachePath ) ) < 0 ) {
        return status;
    }

    /* fd = creat (cachePath, mode); WRONLY */
    fd = open( cachePath, O_CREAT | O_EXCL | O_RDWR, mode );
    if ( fd < 0 ) {
        rodsLog( LOG_ERROR,
                 "irodsMknodWithCache: local cache creat error for %s, errno = %d",
                 cachePath, errno );
        return errno ? ( -1 * errno ) : -1;
    }
    else {
        return fd;
    }
}

/* need to call getIFuseConn before calling irodsOpenWithReadCache */


int
dataObjCreateByFusePath( rcComm_t *conn, int mode,
                         char *irodsPath ) {
    dataObjInp_t dataObjInp;
    int status;

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, irodsPath, MAX_NAME_LEN );

    if ( strlen( MyRodsEnv.rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjInp.condInput, RESC_NAME_KW,
                   MyRodsEnv.rodsDefResource );
    }

    addKeyVal( &dataObjInp.condInput, DATA_TYPE_KW, "generic" );
    /* dataObjInp.createMode = DEF_FILE_CREATE_MODE; */
    dataObjInp.createMode = mode;
    dataObjInp.openFlags = O_RDWR;
    dataObjInp.dataSize = -1;

    status = rcDataObjCreate( conn, &dataObjInp );
    clearKeyVal( &dataObjInp.condInput );

    return status;
}

int
renameLocalPath( PathCacheTable *pctable, char *from, char *to, char *toIrodsPath ) {

    /* do not check existing path here as path cache may be out of date */
    pathCache_t *fromPathCache = matchPathCache( pctable, from );
    if ( NULL == fromPathCache ) {
        return 0;
    }
    LOCK_STRUCT( *fromPathCache );

    if ( fromPathCache->fileCache != NULL ) {
        LOCK_STRUCT( *( fromPathCache->fileCache ) );
        free( fromPathCache->fileCache->localPath );
        fromPathCache->fileCache->localPath = strdup( to );
        free( fromPathCache->fileCache->objPath );
        fromPathCache->fileCache->objPath = strdup( toIrodsPath );
        UNLOCK_STRUCT( *( fromPathCache->fileCache ) );
    }

    pathCache_t *tmpPathCache = NULL;
    LOCK( *pctable -> PathCacheLock );
    pathReplace( pctable, ( char * ) to, fromPathCache->fileCache, &fromPathCache->stbuf, &tmpPathCache );

    pathNotExist( pctable, ( char * ) from );

    UNLOCK( *pctable -> PathCacheLock );
    UNLOCK_STRUCT( *fromPathCache );
    return 0;
}


