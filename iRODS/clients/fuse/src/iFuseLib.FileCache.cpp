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
#include "list.hpp"
#include "iFuseLib.Lock.hpp"

/* Hashtable *NewlyCreatedFileTable; */
concurrentList_t* FileCacheList;
char *ReadCacheDir = NULL;
char FuseCacheDir[MAX_NAME_LEN];

int initFileCache() {
    FileCacheList = newConcurrentList();
    return 0;
}
int iFuseFileCacheLseek( fileCache_t *fileCache, off_t offset ) {
    LOCK_STRUCT( *fileCache );
    int status = _iFuseFileCacheLseek( fileCache, offset );
    UNLOCK_STRUCT( *fileCache );
    return status;
}

int _iFuseFileCacheLseek( fileCache_t *fileCache, off_t offset ) {

    int status;
    if ( offset == fileCache->offset ) {
        return 0;
    }
    if ( fileCache->state == NO_FILE_CACHE ) {
        iFuseConn_t *conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
        openedDataObjInp_t dataObjLseekInp;
        fileLseekOut_t *dataObjLseekOut = NULL;

        bzero( &dataObjLseekInp, sizeof( dataObjLseekInp ) );
        dataObjLseekInp.l1descInx = fileCache->iFd;
        dataObjLseekInp.offset = offset;
        dataObjLseekInp.whence = SEEK_SET;

        status = rcDataObjLseek( conn->conn, &dataObjLseekInp, &dataObjLseekOut );
        unuseIFuseConn( conn );
        if ( dataObjLseekOut != NULL ) {
            free( dataObjLseekOut );
        }

    }
    else {
        rodsLong_t lstatus;
        lstatus = lseek( fileCache->iFd, offset, SEEK_SET );
        if ( lstatus >= 0 ) {
            status = 0;
        }
        else {
            status = lstatus;
        }
    }
    fileCache->offset = offset;

    return status;
}

int iFuseFileCacheFlush( fileCache_t *fileCache ) {
    int status;

    LOCK_STRUCT( *fileCache );
    status = _iFuseFileCacheFlush( fileCache );
    UNLOCK_STRUCT( *fileCache );
    return status;
}

int _iFuseFileCacheFlush( fileCache_t *fileCache ) {
    int status = 0;
    int objFd;
    /* simply return if no file cache or the file cache hasn't been updated */
    if ( fileCache->state != HAVE_NEWLY_CREATED_CACHE ) {
        UNLOCK_STRUCT( *fileCache );
        return 0;
    }

    /* no need to flush cache file as we are using low-level io */
    /*status = fsync (fileCache->iFd);
    if (status < 0) {
    	status = (errno ? (-1 * errno) : -1);
    	rodsLog (LOG_ERROR,
    			"ifuseFlush: flush of cache file for %s error, status = %d",
    			fileCache->localPath, status);
    	return -EBADF;
    }*/

    struct stat stbuf;
    stat( fileCache->fileCachePath, &stbuf );
    /* put cache file to server */
    UNLOCK_STRUCT( *fileCache );
    iFuseConn_t *conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
    LOCK_STRUCT( *fileCache );

    RECONNECT_IF_NECESSARY( status, conn, ifusePut( conn->conn, fileCache->objPath, fileCache->fileCachePath, fileCache->mode, stbuf.st_size ) );
    unuseIFuseConn( conn );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "ifuseClose: ifusePut of %s error, status = %d",
                 fileCache->localPath, status );
        status = -EBADF;
        return status;
    }

    objFd = status;

    if ( stbuf.st_size > MAX_READ_CACHE_SIZE ) {
        /* too big to keep */
        /* close cache file */
        status = close( fileCache->iFd );
        if ( status < 0 ) {
            status = ( errno ? ( -1 * errno ) : -1 );
            rodsLog( LOG_ERROR,
                     "ifuseClose: close of cache file for %s error, status = %d",
                     fileCache->localPath, status );
            return -EBADF;
        }
        fileCache->iFd = objFd;
        fileCache->state = NO_FILE_CACHE;
        fileCache->offset = 0;
    }
    else {
        fileCache->state = HAVE_READ_CACHE;
    }


    /*	UNLOCK_STRUCT(*fileCache);

    	pathCache_t *tmpPathCache;
    	if (matchAndLockPathCache (fileCache->localPath, &tmpPathCache) == 1) {
    		tmpPathCache->stbuf.st_size = stbuf.st_size;
    		UNLOCK_STRUCT(*tmpPathCache);
    	}
    	LOCK_STRUCT(*fileCache);
    */
    /*	rodsLog (LOG_ERROR,
    			"ifuseClose: IFuseDesc indicated a newly created cache, but does not exist for %s",
    			path);
    	savedStatus = -EBADF; */

    return ( status );
}
int ifuseFileCacheSwapOut( fileCache_t *fileCache ) {
    int status = 0;
    if ( fileCache == NULL ) { return USER__NULL_INPUT_ERR; }
    LOCK_STRUCT( *fileCache );

    /* flush local cache file to remote server */
    int objFd;

    LOCK_STRUCT( *fileCache );
    /* simply return if no file cache or the file cache hasn't been updated */
    if ( fileCache->state != HAVE_NEWLY_CREATED_CACHE ) {
        UNLOCK_STRUCT( *fileCache );
        return 0;
    }

    /* no need to flush cache file as we are using low-level io */
    /* status = fsync (fileCache->iFd);
    if (status < 0) {
    	status = (errno ? (-1 * errno) : -1);
    	rodsLog (LOG_ERROR,
    			"ifuseFlush: flush of cache file for %s error, status = %d",
    			fileCache->localPath, status);
    	return -EBADF;
    }*/

    struct stat stbuf;
    stat( fileCache->fileCachePath, &stbuf );
    /* put cache file to server */
    iFuseConn_t *conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
    RECONNECT_IF_NECESSARY( status, conn, ifusePut( conn->conn, fileCache->objPath, fileCache->fileCachePath, fileCache->mode, stbuf.st_size ) );
    unuseIFuseConn( conn );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "ifuseClose: ifusePut of %s error, status = %d",
                 fileCache->localPath, status );
        status = -EBADF;
        UNLOCK_STRUCT( *fileCache );
        return status;
    }

    objFd = status;

    /* close cache file */
    status = close( fileCache->iFd );
    if ( status < 0 ) {
        status = ( errno ? ( -1 * errno ) : -1 );
        rodsLog( LOG_ERROR,
                 "ifuseClose: close of cache file for %s error, status = %d",
                 fileCache->localPath, status );
        UNLOCK_STRUCT( *fileCache );
        return -EBADF;
    }
    fileCache->iFd = objFd;
    fileCache->state = NO_FILE_CACHE;

    /*	UNLOCK_STRUCT(*fileCache);

    	pathCache_t *tmpPathCache;
    	if (matchAndLockPathCache (fileCache->localPath, &tmpPathCache) == 1) {
    		tmpPathCache->stbuf.st_size = stbuf.st_size;
    		UNLOCK_STRUCT(*tmpPathCache);
    	}
    	*/
    /*	rodsLog (LOG_ERROR,
    			"ifuseClose: IFuseDesc indicated a newly created cache, but does not exist for %s",
    			path);
    	savedStatus = -EBADF; */

    return ( status );

}
/* close any open files inside the file cache */
int ifuseFileCacheClose( fileCache_t *fileCache ) {
    int status = 0;
    LOCK_STRUCT( *fileCache );
    if ( fileCache->state == NO_FILE_CACHE ) {
        /* close remote file */
        iFuseConn_t *conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
        status = closeIrodsFd( conn->conn, fileCache->iFd );
        unuseIFuseConn( conn );
        fileCache->offset = 0;
    }
    else {
        /* close local file */
        if ( fileCache->status == 1 ) {
            status = close( fileCache->iFd );
            fileCache->iFd = 0;
            fileCache->offset = 0;
        }
    }
    UNLOCK_STRUCT( *fileCache );
    return status;
}

fileCache_t *addFileCache( int iFd, char *objPath, char *localPath, char *cachePath, int mode, rodsLong_t fileSize, cacheState_t state ) {
    uint cachedTime = time( 0 );
    fileCache_t *fileCache, *swapCache;

    if ( state == NO_FILE_CACHE ) {
        return newFileCache( iFd, objPath, localPath, cachePath, cachedTime, mode, fileSize, state );
    }
    else {

        fileCache = newFileCache( iFd, objPath, localPath, cachePath, cachedTime, mode, fileSize, state );
        LOCK_STRUCT( *FileCacheList );

        if ( _listSize( FileCacheList ) >= NUM_NEWLY_CREATED_SLOT ) {
            ListNode *node = FileCacheList->list->head;
            swapCache = ( fileCache_t * ) node->value;
            listRemoveNoRegion( FileCacheList->list, node );
            ifuseFileCacheSwapOut( swapCache );

            listAppendNoRegion( FileCacheList->list, fileCache );

        }

        UNLOCK_STRUCT( *FileCacheList );
        return fileCache;
    }
}

int
getFileCachePath( const char *inPath, char *cacehPath ) {
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    struct stat statbuf;

    if ( inPath == NULL || cacehPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "getFileCachePath: input inPath or cacehPath is NULL" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
    splitPathByKey( ( char * ) inPath, myDir, myFile, '/' );

    while ( 1 ) {
        snprintf( cacehPath, MAX_NAME_LEN, "%s/%s.%d", FuseCacheDir,
                  myFile, ( int ) random() );
        if ( stat( cacehPath, &statbuf ) < 0 ) { break; }
    }
    return 0;
}

int
setAndMkFileCacheDir() {
    char *tmpStr, *tmpDir;
    struct passwd *myPasswd;
    int status;

    myPasswd = getpwuid( getuid() );

    if ( ( tmpStr = getenv( "FuseCacheDir" ) ) != NULL && strlen( tmpStr ) > 0 ) {
        tmpDir = tmpStr;
    }
    else {
        tmpDir = FUSE_CACHE_DIR;
    }

    snprintf( FuseCacheDir, MAX_NAME_LEN, "%s/%s.%d", tmpDir,
              myPasswd->pw_name, getpid() );

    if ( ( status = mkdirR( "/", FuseCacheDir, DEF_DIR_MODE ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "setAndMkFileCacheDir: mkdirR of %s error. status = %d",
                 FuseCacheDir, status );
    }

    return ( status );

}

int _ifuseFileCacheWrite( fileCache_t *fileCache, char *buf, size_t size, off_t offset ) {

    int status, myError;
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;
    iFuseConn_t *conn;

    bzero( &dataObjWriteInp, sizeof( dataObjWriteInp ) );
    /* lseek to the right offset in case this cache is share by multiple descs */
    status = _iFuseFileCacheLseek( fileCache, offset );
    if ( status < 0 ) {
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return ( -myError );
        }
        else {
            return -ENOENT;
        }
    }


    if ( fileCache->state == NO_FILE_CACHE ) {
        /* no file cache */
        dataObjWriteInpBBuf.buf = ( void * ) buf;
        dataObjWriteInpBBuf.len = size;
        dataObjWriteInp.l1descInx = fileCache->iFd;
        dataObjWriteInp.len = size;

        conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
        status = rcDataObjWrite( conn->conn, &dataObjWriteInp, &dataObjWriteInpBBuf );
        unuseIFuseConn( conn );
        if ( status < 0 ) {
            if ( ( myError = getErrno( status ) ) > 0 ) {
                return ( -myError );
            }
            else {
                return -ENOENT;
            }
        }
        else if ( status != ( int ) size ) {
            rodsLog( LOG_ERROR,
                     "ifuseWrite: IFuseDesc[descInx].conn for %s is NULL", fileCache->localPath );
            return -ENOENT;
        }
        fileCache->offset += status;
        if ( fileCache->offset > fileCache->fileSize ) {
            fileCache->fileSize = fileCache->offset;
        }
    }
    else {
        status = write( fileCache->iFd, buf, size );

        if ( status < 0 ) { return ( errno ? ( -1 * errno ) : -1 ); }
        fileCache->offset += status;
        if ( fileCache->offset > fileCache->fileSize ) {
            fileCache->fileSize = fileCache->offset;
        }
        if ( fileCache->offset >= MAX_NEWLY_CREATED_CACHE_SIZE ) {
            _iFuseFileCacheFlush( fileCache );
            fileCache->iFd = 0;
            /* reopen file */
            dataObjInp_t dataObjOpenInp;
            memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );
            rstrcpy( dataObjOpenInp.objPath, fileCache->objPath, MAX_NAME_LEN );
            dataObjOpenInp.openFlags = O_RDWR;

            conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );
            status = rcDataObjOpen( conn->conn, &dataObjOpenInp );
            unuseIFuseConn( conn );

            if ( status < 0 ) {
                rodsLog( LOG_ERROR, "iFuseWrtie: rcDataObjOpen of %s error. status = %d", fileCache->objPath, status );
                return -ENOENT;
            }

            fileCache->iFd = status;
        }
    }
    return status;
}


int ifuseFileCacheWrite( fileCache_t *fileCache, char *buf, size_t size, off_t offset ) {
    LOCK_STRUCT( *fileCache );
    int status = _ifuseFileCacheWrite( fileCache, buf, size, offset );
    UNLOCK_STRUCT( *fileCache );
    return status;
}
int _ifuseFileCacheRead( fileCache_t *fileCache, char *buf, size_t size, off_t offset ) {
    int status, myError;

    status = _iFuseFileCacheLseek( fileCache, offset );
    if ( status < 0 ) {
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return ( -myError );
        }
        else {
            return -ENOENT;
        }
    }

    if ( fileCache->state == NO_FILE_CACHE ) {
        iFuseConn_t *conn;
        openedDataObjInp_t dataObjReadInp;
        bytesBuf_t dataObjReadOutBBuf;
        int myError;


        bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
        dataObjReadOutBBuf.buf = buf;
        dataObjReadOutBBuf.len = size;
        dataObjReadInp.l1descInx = fileCache->iFd;
        dataObjReadInp.len = size;

        conn = getAndUseConnByPath( fileCache->localPath, &MyRodsEnv, &status );

        status = rcDataObjRead( conn->conn,
                                &dataObjReadInp, &dataObjReadOutBBuf );
        unuseIFuseConn( conn );
        if ( status < 0 ) {
            if ( ( myError = getErrno( status ) ) > 0 ) {
                return ( -myError );
            }
            else {
                return -ENOENT;
            }
        }
    }
    else {
        status = read( fileCache->iFd, buf, size );

        if ( status < 0 ) { return ( errno ? ( -1 * errno ) : -1 ); }
    }
    fileCache->offset += status;

    return status;
}
int ifuseFileCacheRead( fileCache_t *fileCache, char *buf, size_t size, off_t offset ) {
    LOCK_STRUCT( *fileCache );
    int status = _ifuseFileCacheRead( fileCache, buf, size, offset );
    UNLOCK_STRUCT( *fileCache );
    return status;
}
