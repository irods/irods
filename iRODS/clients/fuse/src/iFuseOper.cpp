/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.hpp"
#include "iFuseOper.hpp"
#include "miscUtil.hpp"
#include "sockComm.hpp"

PathCacheTable *pctable = initPathCache();

int
irodsGetattr( const char *path, struct stat *stbuf ) {
    int status;
    iFuseConn_t *iFuseConn = NULL;

#ifdef CACHE_FUSE_PATH

    /*if (lookupPathNotExist( (char *) path) == 1) {
        rodsLog (LOG_DEBUG, "irodsGetattr: a match for non existing path %s", path);
        return -ENOENT;
    }*/

    pathCache_t *tmpPathCache = matchPathCache( pctable, path );
    if ( NULL != tmpPathCache ) {
        LOCK_STRUCT( *tmpPathCache );
        rodsLog( LOG_DEBUG, "irodsGetattr: a match for path %s", path );
        if ( tmpPathCache->fileCache != NULL ) {
            LOCK_STRUCT( *( tmpPathCache->fileCache ) );
            if ( tmpPathCache->fileCache->state == HAVE_NEWLY_CREATED_CACHE ) {
                status = updatePathCacheStatFromFileCache( tmpPathCache );
                UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
                if ( status < 0 ) {
                    clearPathFromCache( pctable, ( char * ) path );
                }
                else {
                    *stbuf = tmpPathCache->stbuf;
                    UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
                    UNLOCK_STRUCT( *tmpPathCache );
                    return 0;
                }
            }
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
        }
        UNLOCK_STRUCT( *tmpPathCache );
    }
#endif
    iFuseConn = getAndUseConnByPath( ( char * ) path, &status );
    status = _irodsGetattr( iFuseConn, path, stbuf );
    unuseIFuseConn( iFuseConn );
#ifdef CACHE_FUSE_PATH
    if ( status == -ENOENT ) {
        pathNotExist( pctable, ( char * ) path );
    }
    else {
        /* don't set file cache */
        pathExist( pctable, ( char * ) path, NULL, stbuf, &tmpPathCache );
    }
#endif

    return status;
}

int
_irodsGetattr( iFuseConn_t *iFuseConn, const char *path, struct stat *stbuf ) {
    int status;
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = NULL;

    rodsLog( LOG_DEBUG, "_irodsGetattr: %s", path );

    memset( stbuf, 0, sizeof( struct stat ) );
    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv, dataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsGetattr: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }
    status = rcObjStat( iFuseConn->conn, &dataObjInp, &rodsObjStatOut );
    if ( status < 0 ) {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcObjStat( iFuseConn->conn, &dataObjInp, &rodsObjStatOut );
        }
        if ( status < 0 ) {
            if ( status != USER_FILE_DOES_NOT_EXIST ) {
                rodsLogError( LOG_ERROR, status,
                              "irodsGetattr: rcObjStat of %s error", path );
            }
            return -ENOENT;
        }
    }


    if ( rodsObjStatOut->objType == COLL_OBJ_T ) {
        fillDirStat( stbuf,
                     atoi( rodsObjStatOut->createTime ), atoi( rodsObjStatOut->modifyTime ),
                     atoi( rodsObjStatOut->modifyTime ) );
    }
    else if ( rodsObjStatOut->objType == UNKNOWN_OBJ_T ) {

        freeRodsObjStat( rodsObjStatOut );
        return -ENOENT;
    }
    else {
        fillFileStat( stbuf, rodsObjStatOut->dataMode, rodsObjStatOut->objSize,
                      atoi( rodsObjStatOut->createTime ), atoi( rodsObjStatOut->modifyTime ),
                      atoi( rodsObjStatOut->modifyTime ) );
    }

    freeRodsObjStat( rodsObjStatOut );

    return 0;
}

int
irodsReadlink( const char *path, char *buf, size_t size ) {
    int status;
    iFuseConn_t *iFuseConn = NULL;
    int l1descInx;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjReadInp;
    bytesBuf_t dataObjReadOutBBuf;
    char collPath[MAX_NAME_LEN];

    rodsLog( LOG_DEBUG, "irodsReadlink: %s", path );

    status = parseRodsPathStr( ( char * )( path + 1 ), &MyRodsEnv, collPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsReaddir: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    iFuseConn = getAndUseConnByPath( ( char * ) path, &status );

    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );

    rstrcpy( dataObjOpenInp.objPath, collPath, MAX_NAME_LEN );
    dataObjOpenInp.openFlags = O_RDONLY;

    status = rcDataObjOpen( iFuseConn->conn, &dataObjOpenInp );

    if ( status < 0 ) {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcDataObjOpen( iFuseConn->conn, &dataObjOpenInp );
        }
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "irodsReadlink: rcDataObjOpen of %s error. status = %d", collPath, status );
            unuseIFuseConn( iFuseConn );
            return -ENOENT;
        }
    }

    l1descInx = status;

    memset( &dataObjReadInp, 0, sizeof( dataObjReadInp ) );
    memset( &dataObjReadOutBBuf, 0, sizeof( bytesBuf_t ) );

    dataObjReadInp.l1descInx = l1descInx;
    dataObjReadInp.len = size - 1;
    status = rcDataObjRead( iFuseConn->conn, &dataObjReadInp, &dataObjReadOutBBuf );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "irodsReadlink: rcDataObjRead of %s error. status = %d", collPath, status );
        unuseIFuseConn( iFuseConn );
        return -ENOENT;
    }

    memcpy( buf, dataObjReadOutBBuf.buf, status );
    buf[status] = '\0';
    rcDataObjClose( iFuseConn->conn, &dataObjReadInp );
    unuseIFuseConn( iFuseConn );

    return 0;
}

int
irodsReaddir( const char *path, void *buf, fuse_fill_dir_t filler,
              off_t offset, struct fuse_file_info *fi ) {
    char collPath[MAX_NAME_LEN];
    collHandle_t collHandle;
    collEnt_t collEnt;
    int status;
#ifdef CACHE_FUSE_PATH
    struct stat stbuf;
    pathCache_t *tmpPathCache;
#endif
    /* don't know why we need this. the example have them */
    ( void ) offset;
    ( void ) fi;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsReaddir: %s", path );

    filler( buf, ".", NULL, 0 );
    filler( buf, "..", NULL, 0 );

    status = parseRodsPathStr( ( char * )( path + 1 ), &MyRodsEnv, collPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsReaddir: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    iFuseConn = getAndUseConnByPath( ( char * ) path, &status );
    status = rclOpenCollection( iFuseConn->conn, collPath, 0, &collHandle );

    if ( status < 0 ) {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rclOpenCollection( iFuseConn->conn, collPath, 0,
                                        &collHandle );
        }
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "irodsReaddir: rclOpenCollection of %s error. status = %d",
                     collPath, status );
            unuseIFuseConn( iFuseConn );
            return -ENOENT;
        }
    }
    while ( ( status = rclReadCollection( iFuseConn->conn, &collHandle, &collEnt ) )
            >= 0 ) {
        char myDir[MAX_NAME_LEN], mySubDir[MAX_NAME_LEN];
#ifdef CACHE_FUSE_PATH
        char childPath[MAX_NAME_LEN];

        bzero( &stbuf, sizeof( struct stat ) );
#endif
        if ( collEnt.objType == DATA_OBJ_T ) {
            filler( buf, collEnt.dataName, NULL, 0 );
#ifdef CACHE_FUSE_PATH
            if ( strcmp( path, "/" ) == 0 ) {
                snprintf( childPath, MAX_NAME_LEN, "/%s", collEnt.dataName );
            }
            else {
                snprintf( childPath, MAX_NAME_LEN, "%s/%s",
                          path, collEnt.dataName );
            }
            if ( lookupPathExist( pctable, ( char * ) childPath, &tmpPathCache ) != 1 ) {
                fillFileStat( &stbuf, collEnt.dataMode, collEnt.dataSize,
                              atoi( collEnt.createTime ), atoi( collEnt.modifyTime ),
                              atoi( collEnt.modifyTime ) );
                pathExist( pctable, childPath, NULL, &stbuf, &tmpPathCache );
            }
#endif
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            splitPathByKey( collEnt.collName, myDir, MAX_NAME_LEN, mySubDir, MAX_NAME_LEN, '/' );
            if ( mySubDir[0] != '\0' ) {
                filler( buf, mySubDir, NULL, 0 );
#ifdef CACHE_FUSE_PATH
                if ( strcmp( path, "/" ) == 0 ) {
                    snprintf( childPath, MAX_NAME_LEN, "/%s", mySubDir );
                }
                else {
                    snprintf( childPath, MAX_NAME_LEN, "%s/%s", path, mySubDir );
                }
                if ( lookupPathExist( pctable, ( char * ) childPath, &tmpPathCache ) != 1 ) {
                    fillDirStat( &stbuf,
                                 atoi( collEnt.createTime ), atoi( collEnt.modifyTime ),
                                 atoi( collEnt.modifyTime ) );
                    pathExist( pctable, childPath, NULL, &stbuf, &tmpPathCache );
                }
#endif
            }
        }
    }
    rclCloseCollection( &collHandle );
    unuseIFuseConn( iFuseConn );

    return 0;
}

int
irodsMknod( const char *path, mode_t mode, dev_t ) {
    pathCache_t *tmpPathCache = NULL;
    struct stat stbuf;
    int status = -1;
    char cachePath[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    iFuseConn_t *iFuseConn = NULL;
    fileCache_t *fileCache = NULL;
    /* iFuseDesc_t *desc = NULL; */

    rodsLog( LOG_DEBUG, "irodsMknod: %s", path );

    if ( irodsGetattr( path, &stbuf ) >= 0 ) {
        return -EEXIST;
    }

    int localFd = irodsMknodWithCache( ( char * )path, mode, cachePath );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv, objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "dataObjCreateByFusePath: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }


    status = getAndUseIFuseConn( &iFuseConn );
    if ( status < 0 ) {
        status = dataObjCreateByFusePath( iFuseConn->conn,
                                          mode, objPath );

        if ( status < 0 ) {
            if ( isReadMsgError( status ) ) {
                ifuseReconnect( iFuseConn );
                status = dataObjCreateByFusePath( iFuseConn->conn, mode, objPath );
            }
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "irodsMknod: rcDataObjCreate of %s error", path );
                unuseIFuseConn( iFuseConn );
                if ( localFd >= 0 ) {
                    close( localFd );
                }
                return -ENOENT;
            }
        }
    }
    fileCache = addFileCache( localFd, objPath, ( char * ) path, cachePath, mode, 0, HAVE_NEWLY_CREATED_CACHE );
    stbuf.st_mode = mode;
    pathExist( pctable, ( char * ) path, fileCache, &stbuf, &tmpPathCache );

    unuseIFuseConn( iFuseConn );

    return 0;
}

int
irodsMkdir( const char *path, mode_t ) {
    collInp_t collCreateInp;
    int status;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsMkdir: %s", path );

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );

    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               collCreateInp.collName );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsMkdir: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    getAndUseIFuseConn( &iFuseConn );
    status = rcCollCreate( iFuseConn->conn, &collCreateInp );

    if ( status < 0 ) {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcCollCreate( iFuseConn->conn, &collCreateInp );
        }
        unuseIFuseConn( iFuseConn );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsMkdir: rcCollCreate of %s error", path );
            return -ENOENT;
        }
#ifdef CACHE_FUSE_PATH
    }
    else {
        struct stat stbuf;
        uint mytime = time( 0 );
        bzero( &stbuf, sizeof( struct stat ) );
        fillDirStat( &stbuf, mytime, mytime, mytime );
        pathExist( pctable, ( char * ) path, NULL, &stbuf, NULL );
#endif
        unuseIFuseConn( iFuseConn );
    }

    return 0;
}

int
irodsUnlink( const char *path ) {
    dataObjInp_t dataObjInp;
    int status;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsUnlink: %s", path );

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );

    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               dataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsUnlink: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );

    getAndUseIFuseConn( &iFuseConn );
    status = rcDataObjUnlink( iFuseConn->conn, &dataObjInp );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathNotExist( pctable, ( char * ) path );
#endif
        status = 0;
    }
    else {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcDataObjUnlink( iFuseConn->conn, &dataObjInp );
        }
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsUnlink: rcDataObjUnlink of %s error", path );
            status = -ENOENT;
        }
    }
    unuseIFuseConn( iFuseConn );

    clearKeyVal( &dataObjInp.condInput );

    return status;
}

int
irodsRmdir( const char *path ) {
    collInp_t collInp;
    int status;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsRmdir: %s", path );

    memset( &collInp, 0, sizeof( collInp ) );

    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               collInp.collName );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsRmdir: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    addKeyVal( &collInp.condInput, FORCE_FLAG_KW, "" );

    getAndUseIFuseConn( &iFuseConn );
    RECONNECT_IF_NECESSARY( status, iFuseConn, rcRmColl( iFuseConn->conn, &collInp, 0 ) );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathNotExist( pctable, ( char * ) path );
#endif
        status = 0;
    }
    else {
        rodsLogError( LOG_ERROR, status,
                      "irodsRmdir: rcRmColl of %s error", path );
        status = -ENOENT;
    }

    unuseIFuseConn( iFuseConn );

    clearKeyVal( &collInp.condInput );

    return status;
}

int
irodsSymlink( const char *to, const char *from ) {
    int status;
    iFuseConn_t *iFuseConn = NULL;
    int l1descInx;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteOutBBuf;
    char collPath[MAX_NAME_LEN];
    struct stat stbuf;

    rodsLog( LOG_DEBUG, "irodsSymlink: %s to %s", from, to );

    status = parseRodsPathStr( ( char * )( from + 1 ), &MyRodsEnv, collPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsReaddir: parseRodsPathStr of %s error", from );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    iFuseConn = getAndUseConnByPath( ( char * ) from, &status );
    status = _irodsGetattr( iFuseConn, ( char * ) from, &stbuf );

    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );
    rstrcpy( dataObjOpenInp.objPath, collPath, MAX_NAME_LEN );
    if ( status != -ENOENT ) {
        if ( status < 0 ) {
            return status;
        }
        dataObjOpenInp.dataSize = 0;

        status = rcDataObjTruncate( iFuseConn->conn, &dataObjOpenInp );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "irodsReadlink: rcDataObjTruncate of %s error. status = %d", collPath, status );
            unuseIFuseConn( iFuseConn );
            return -ENOENT;
        }
    }

    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );
    rstrcpy( dataObjOpenInp.objPath, collPath, MAX_NAME_LEN );
    dataObjOpenInp.openFlags = O_WRONLY | O_CREAT;
    dataObjOpenInp.createMode = S_IFLNK;

    status = rcDataObjOpen( iFuseConn->conn, &dataObjOpenInp );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "irodsSymlink: rcDataObjOpen of %s error. status = %d", collPath, status );
        unuseIFuseConn( iFuseConn );
        return -ENOENT;
    }

    l1descInx = status;

    memset( &dataObjWriteInp, 0, sizeof( dataObjWriteInp ) );
    memset( &dataObjWriteOutBBuf, 0, sizeof( bytesBuf_t ) );

    dataObjWriteInp.l1descInx = l1descInx;
    dataObjWriteInp.len = strlen( to );

    dataObjWriteOutBBuf.len = strlen( to );
    dataObjWriteOutBBuf.buf = strdup( to );

    status = rcDataObjWrite( iFuseConn->conn, &dataObjWriteInp, &dataObjWriteOutBBuf );
    free( dataObjWriteOutBBuf.buf );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "irodsSymlink: rcDataObjWrite of %s error. status = %d", collPath, status );
        unuseIFuseConn( iFuseConn );
        return -ENOENT;
    }

    rcDataObjClose( iFuseConn->conn, &dataObjWriteInp );
    unuseIFuseConn( iFuseConn );

    return 0;
}

int
irodsRename( const char *from, const char *to ) {
    dataObjCopyInp_t dataObjRenameInp;
    int status;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsRename: %s to %s", from, to );

    /* test rcDataObjRename */

    memset( &dataObjRenameInp, 0, sizeof( dataObjRenameInp ) );

    status = parseRodsPathStr( ( char * )( from + 1 ) , &MyRodsEnv,
                               dataObjRenameInp.srcDataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsRename: parseRodsPathStr of %s error", from );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    status = parseRodsPathStr( ( char * )( to + 1 ) , &MyRodsEnv,
                               dataObjRenameInp.destDataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsRename: parseRodsPathStr of %s error", to );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    char *toIrodsPath = strdup( dataObjRenameInp.destDataObjInp.objPath );

    addKeyVal( &dataObjRenameInp.destDataObjInp.condInput, FORCE_FLAG_KW, "" );

    dataObjRenameInp.srcDataObjInp.oprType =
        dataObjRenameInp.destDataObjInp.oprType = RENAME_UNKNOWN_TYPE;

    getAndUseIFuseConn( &iFuseConn );
    /*    rodsLog (LOG_ERROR, "irodsrename: %s -> %s conn: %p", from, to, iFuseConn);*/

    status = rcDataObjRename( iFuseConn->conn, &dataObjRenameInp );

    if ( status == CAT_NAME_EXISTS_AS_DATAOBJ ||
            status == SYS_DEST_SPEC_COLL_SUB_EXIST ) {
        rcDataObjUnlink( iFuseConn->conn, &dataObjRenameInp.destDataObjInp );
        status = rcDataObjRename( iFuseConn->conn, &dataObjRenameInp );
    }

    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        status = renameLocalPath( pctable, ( char * ) from, ( char * ) to, ( char * ) toIrodsPath );
#endif
    }
    else {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcDataObjRename( iFuseConn->conn, &dataObjRenameInp );
        }
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsRename: rcDataObjRename of %s to %s error", from, to );
            status = -ENOENT;
        }
    }
    unuseIFuseConn( iFuseConn );
    free( toIrodsPath );

    return status;
}

int
irodsLink( const char *from, const char *to ) {
    rodsLog( LOG_DEBUG, "irodsLink: %s to %s", from, to );
    return 0;
}

int
irodsChmod( const char *path, mode_t mode ) {
    int status;
    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;
    char dataMode[SHORT_STR_LEN];
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsChmod: %s", path );

    pathCache_t *tmpPathCache = matchPathCache( pctable, path );

    if ( NULL != tmpPathCache ) {
        LOCK_STRUCT( *tmpPathCache );
        if ( tmpPathCache->fileCache != NULL ) {
            LOCK_STRUCT( *tmpPathCache->fileCache );
            if ( tmpPathCache->fileCache->state == HAVE_NEWLY_CREATED_CACHE ) {
                /* has not actually been created yet */
                tmpPathCache->fileCache->mode = mode;
                UNLOCK_STRUCT( *tmpPathCache->fileCache );
                UNLOCK_STRUCT( *tmpPathCache );
                return 0;
            }
            UNLOCK_STRUCT( *tmpPathCache->fileCache );
        }
        UNLOCK_STRUCT( *tmpPathCache );
        if ( tmpPathCache->stbuf.st_nlink != 1 ) {
            rodsLog( LOG_NOTICE,
                     "irodsChmod: modification of the mode of non file object is currently not supported", path );
            return 0;
        }
    }


    memset( &regParam, 0, sizeof( regParam ) );
    snprintf( dataMode, SHORT_STR_LEN, "%d", mode );
    addKeyVal( &regParam, DATA_MODE_KW, dataMode );
    addKeyVal( &regParam, ALL_KW, "" );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               dataObjInfo.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsChmod: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    modDataObjMetaInp.regParam = &regParam;
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;

    getAndUseIFuseConn( &iFuseConn );
    RECONNECT_IF_NECESSARY( status, iFuseConn, rcModDataObjMeta( iFuseConn->conn, &modDataObjMetaInp ) );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathCache_t *tmpPathCache = matchPathCache( pctable, path );
        if ( NULL != tmpPathCache ) {
            LOCK_STRUCT( *tmpPathCache );
            tmpPathCache->stbuf.st_mode &= 0xfffffe00;
            tmpPathCache->stbuf.st_mode |= ( mode & 0777 );
            UNLOCK_STRUCT( *tmpPathCache );
        }
#endif
        status = 0;
    }
    else {
        rodsLogError( LOG_ERROR, status,
                      "irodsChmod: rcModDataObjMeta failure" );
        status = -ENOENT;
    }

    unuseIFuseConn( iFuseConn );
    clearKeyVal( &regParam );

    return status;
}

int
irodsChown( const char *path, uid_t, gid_t ) {
    rodsLog( LOG_DEBUG, "irodsChown: %s", path );
    return 0;
}

int
irodsTruncate( const char *path, off_t size ) {
    dataObjInp_t dataObjInp;
    int status;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsTruncate: %s", path );

    pathCache_t *tmpPathCache = matchPathCache( pctable, path );
    if ( tmpPathCache != NULL ) {
        LOCK_STRUCT( *tmpPathCache );
        if ( tmpPathCache->fileCache != NULL ) {
            LOCK_STRUCT( *tmpPathCache->fileCache );
            if ( tmpPathCache->fileCache->state == HAVE_NEWLY_CREATED_CACHE ) {
                status = truncate( tmpPathCache->fileCache->fileCachePath, size );
                if ( status >= 0 ) {
                    updatePathCacheStatFromFileCache( tmpPathCache );
                    UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
                    UNLOCK_STRUCT( *tmpPathCache );
                    return 0;
                }
            }
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
        }
        UNLOCK_STRUCT( *tmpPathCache );
    }


    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv, dataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "irodsTruncate: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    dataObjInp.dataSize = size;

    getAndUseIFuseConn( &iFuseConn );
    RECONNECT_IF_NECESSARY( status, iFuseConn, rcDataObjTruncate( iFuseConn->conn, &dataObjInp ) );
    if ( status >= 0 ) {

        pathCache_t *tmpPathCache = matchPathCache( pctable, path );
        if ( tmpPathCache != NULL ) {
            LOCK_STRUCT( *tmpPathCache );
            tmpPathCache->stbuf.st_size = size;
            UNLOCK_STRUCT( *tmpPathCache );
        }
        status = 0;
    }
    else {
        rodsLogError( LOG_ERROR, status,
                      "irodsTruncate: rcDataObjTruncate of %s error", path );
        status = -ENOENT;
    }
    unuseIFuseConn( iFuseConn );

    return status;
}

int
irodsFlush( const char *path, struct fuse_file_info * ) {
    rodsLog( LOG_DEBUG, "irodsFlush: %s", path );
    return 0;
}

int
irodsUtimens( const char *path, const struct timespec[2] ) {
    rodsLog( LOG_DEBUG, "irodsUtimens: %s", path );
    return 0;
}

int
irodsOpen( const char *path, struct fuse_file_info *fi ) {
    dataObjInp_t dataObjInp;
    int status;
    int fd;
    iFuseConn_t *iFuseConn = NULL;
    iFuseDesc_t *desc = NULL;
    struct stat stbuf;
    char cachePath[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    int flags = fi->flags;

    rodsLog( LOG_DEBUG, "irodsOpen: %s, flags = %d", path, fi->flags );

    pathCache_t *tmpPathCache = matchPathCache( pctable, path );
    if ( tmpPathCache != NULL ) {
        LOCK_STRUCT( *tmpPathCache );
        if ( tmpPathCache->fileCache != NULL ) {
            LOCK_STRUCT( *( tmpPathCache->fileCache ) );
            if ( tmpPathCache->fileCache->state != NO_FILE_CACHE ) {
                rodsLog( LOG_DEBUG, "irodsOpen: a match for %s", path );
                desc = newIFuseDesc( tmpPathCache->fileCache->objPath, ( char * ) path, tmpPathCache->fileCache, &status );
                if ( status < 0 ) {
                    UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
                    UNLOCK_STRUCT( *tmpPathCache );
                    rodsLogError( LOG_ERROR, status, "irodsOpen: create descriptor of %s error", dataObjInp.objPath );
                    return status;
                }
                fi->fh = desc->index;
                if ( tmpPathCache->fileCache->iFd == 0 ) {
                    tmpPathCache->fileCache->iFd = open( tmpPathCache->fileCache->fileCachePath, O_RDWR );
                }
                UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
                UNLOCK_STRUCT( *tmpPathCache );
                return 0;
            }
            UNLOCK_STRUCT( *( tmpPathCache->fileCache ) );
        }
        UNLOCK_STRUCT( *tmpPathCache );
    }

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    dataObjInp.openFlags = flags;

    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv, objPath );
    rstrcpy( dataObjInp.objPath, objPath, MAX_NAME_LEN );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "irodsOpen: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    iFuseConn = getAndUseConnByPath( ( char * ) path, &status );
    /* status = getAndUseIFuseConn( &iFuseConn ); */
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "irodsOpen: cannot get connection for %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    /* do only O_RDONLY (0) */
    status = _irodsGetattr( iFuseConn, path, &stbuf );
    if ( ( flags & ( O_WRONLY | O_RDWR ) ) != 0 || status < 0 || stbuf.st_size > MAX_READ_CACHE_SIZE ) {
        fd = rcDataObjOpen( iFuseConn->conn, &dataObjInp );
        unuseIFuseConn( iFuseConn );

        if ( fd < 0 ) {
            rodsLogError( LOG_ERROR, status, "irodsOpen: rcDataObjOpen of %s error, status = %d", path, fd );
            return -ENOENT;
        }

        fileCache_t *fileCache = addFileCache( fd, objPath, ( char * ) path, NULL, stbuf.st_mode, stbuf.st_size, NO_FILE_CACHE );
        tmpPathCache  = matchPathCache( pctable, path );
        if ( tmpPathCache != NULL ) {
            LOCK_STRUCT( *tmpPathCache );
            addFileCacheForPath( tmpPathCache, fileCache );
            UNLOCK_STRUCT( *tmpPathCache );
        }
        else {
            pathExist( pctable, ( char * ) path, fileCache, &stbuf, NULL );
        }
        desc = newIFuseDesc( objPath, ( char * ) path, fileCache, &status );
        if ( desc == NULL ) {
            rodsLogError( LOG_ERROR, status, "irodsOpen: allocIFuseDesc of %s error", path );
            return -ENOENT;
        }
    }
    else {
        rodsLog( LOG_DEBUG, "irodsOpenWithReadCache: caching %s", path );
        if ( ( status = getFileCachePath( path, cachePath ) ) < 0 ) {
            unuseIFuseConn( iFuseConn );
            return status;
        }
        /* get the file to local cache */
        dataObjInp.dataSize = stbuf.st_size;

        status = rcDataObjGet( iFuseConn->conn, &dataObjInp, cachePath );
        unuseIFuseConn( iFuseConn );

        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "irodsOpenWithReadCache: rcDataObjGet of %s error", dataObjInp.objPath );
            return status;
        }

        int fd = open( cachePath, O_RDWR );

        fileCache_t *fileCache = addFileCache( fd, objPath, ( char * ) path, cachePath, stbuf.st_mode, stbuf.st_size, HAVE_READ_CACHE );
        tmpPathCache = matchPathCache( pctable, path );
        if ( tmpPathCache != NULL ) {
            LOCK_STRUCT( *tmpPathCache );
            addFileCacheForPath( tmpPathCache, fileCache );
            UNLOCK_STRUCT( *tmpPathCache );
        }
        else {
            pathExist( pctable, ( char * ) path, fileCache, &stbuf, NULL );
        }
        desc = newIFuseDesc( objPath, ( char * ) path, fileCache, &status );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "irodsOpen: create descriptor of %s error", dataObjInp.objPath );
            return status;
        }
    }

    fi->fh = desc->index;

    return 0;
}

int
irodsRead( const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi ) {
    int descInx;
    int status;

    rodsLog( LOG_DEBUG, "irodsRead: %s", path );

    descInx = fi->fh;

    if ( checkFuseDesc( descInx ) < 0 ) {
        return -EBADF;
    }

    status = _ifuseRead( &IFuseDesc[descInx], buf, size, offset );

    return status;
}

int
irodsWrite( const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi ) {
    int descInx;
    int status;

    rodsLog( LOG_DEBUG, "irodsWrite: %s", path );

    descInx = fi->fh;

    if ( checkFuseDesc( descInx ) < 0 ) {
        return -EBADF;
    }

    status = _ifuseWrite( &IFuseDesc[descInx], ( char * )buf, size, offset );
    unlockDesc( descInx );

    return status;
}

int
irodsStatfs( const char *path, struct statvfs *stbuf ) {
    int status;

    rodsLog( LOG_DEBUG, "irodsStatfs: %s", path );

    if ( stbuf == NULL ) {
        return 0;
    }


    /* just fake some number */
    status = statvfs( "/", stbuf );
    if ( status < 0 ) {
        // error cond?
    }

    stbuf->f_bsize = FILE_BLOCK_SZ;
    stbuf->f_blocks = 2000000000;
    stbuf->f_bfree = stbuf->f_bavail = 1000000000;
    stbuf->f_files = 200000000;
    stbuf->f_ffree = stbuf->f_favail = 100000000;
    stbuf->f_fsid = 777;
    stbuf->f_namemax = MAX_NAME_LEN;

    return 0;
}

int
irodsRelease( const char *path, struct fuse_file_info *fi ) {
    int descInx;
    int status, myError;

    rodsLog( LOG_DEBUG, "irodsRelease: %s", path );

    descInx = fi->fh;

    /* if (checkFuseDesc (descInx) < 0) {
        return -EBADF;
    } */

    status = ifuseClose( &IFuseDesc[descInx] );

    if ( status < 0 ) {
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return -myError;
        }
        else {
            return -ENOENT;
        }
    }
    else {
        return 0;
    }
}

int
irodsFsync( const char *path, int, struct fuse_file_info * ) {
    rodsLog( LOG_DEBUG, "irodsFsync: %s", path );
    return 0;
}





