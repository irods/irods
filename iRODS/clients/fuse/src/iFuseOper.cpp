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

extern iFuseConn_t *ConnHead;
extern rodsEnv MyRodsEnv;
extern iFuseDesc_t IFuseDesc[];
extern pathCacheQue_t NonExistPathArray[];
extern pathCacheQue_t PathArray[];

int
irodsGetattr( const char *path, struct stat *stbuf ) {
    int status;
    iFuseConn_t *iFuseConn = NULL;

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    status = _irodsGetattr( iFuseConn, path, stbuf, NULL );
    relIFuseConn( iFuseConn );
    return ( status );
}

int
_irodsGetattr( iFuseConn_t *iFuseConn, const char *path, struct stat *stbuf,
               pathCache_t **outPathCache ) {
    int status;
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = NULL;
#ifdef CACHE_FUSE_PATH
    pathCache_t *nonExistPathCache;
    pathCache_t *tmpPathCache;
#endif

    rodsLog( LOG_DEBUG, "_irodsGetattr: %s", path );

#ifdef CACHE_FUSE_PATH
    if ( outPathCache != NULL ) {
        *outPathCache = NULL;
    }
    if ( matchPathInPathCache( ( char * ) path, NonExistPathArray,
                               &nonExistPathCache ) == 1 ) {
        rodsLog( LOG_DEBUG, "irodsGetattr: a match for non existing path %s",
                 path );
        return -ENOENT;
    }

    if ( matchPathInPathCache( ( char * ) path, PathArray, &tmpPathCache ) == 1 ) {
        rodsLog( LOG_DEBUG, "irodsGetattr: a match for path %s", path );
        status = updatePathCacheStat( tmpPathCache );
        if ( status < 0 ) {
            /* we have a problem */
            rmPathFromCache( ( char * ) path, PathArray );
        }
        else {
            *stbuf = tmpPathCache->stbuf;
            if ( outPathCache != NULL ) {
                *outPathCache = tmpPathCache;
            }
            return ( 0 );
        }
    }
#endif

    memset( stbuf, 0, sizeof( struct stat ) );
    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               dataObjInp.objPath );
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
#ifdef CACHE_FUSE_PATH
            addPathToCache( ( char * ) path, NonExistPathArray, stbuf, NULL );
#endif
            return -ENOENT;
        }
    }

    if ( rodsObjStatOut == NULL ) { // JMC cppcheck - null ptr ref
        rodsLogError( LOG_ERROR, status, "irodsGetattr: rcObjStat of %s error", path );

        return -ENOENT;
    }

    if ( rodsObjStatOut->objType == COLL_OBJ_T ) {
        fillDirStat( stbuf,
                     atoi( rodsObjStatOut->createTime ), atoi( rodsObjStatOut->modifyTime ),
                     atoi( rodsObjStatOut->modifyTime ) );
    }
    else if ( rodsObjStatOut->objType == UNKNOWN_OBJ_T ) {
#ifdef CACHE_FUSE_PATH
        addPathToCache( ( char * ) path, NonExistPathArray, stbuf, NULL );
#endif
        if ( rodsObjStatOut != NULL ) {
            freeRodsObjStat( rodsObjStatOut );
        }
        return -ENOENT;
    }
    else {
        fillFileStat( stbuf, rodsObjStatOut->dataMode, rodsObjStatOut->objSize,
                      atoi( rodsObjStatOut->createTime ), atoi( rodsObjStatOut->modifyTime ),
                      atoi( rodsObjStatOut->modifyTime ) );
    }

    if ( rodsObjStatOut != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
    }

#ifdef CACHE_FUSE_PATH
    addPathToCache( ( char * ) path, PathArray, stbuf, outPathCache );
#endif
    return 0;
}

int
irodsReadlink( const char *path, char *buf, size_t size ) {
    rodsLog( LOG_DEBUG, "irodsReadlink: %s", path );
    return ( 0 );
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

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
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
            relIFuseConn( iFuseConn );
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
            if ( matchPathInPathCache( ( char * ) childPath, PathArray,
                                       &tmpPathCache ) != 1 ) {
                fillFileStat( &stbuf, collEnt.dataMode, collEnt.dataSize,
                              atoi( collEnt.createTime ), atoi( collEnt.modifyTime ),
                              atoi( collEnt.modifyTime ) );
                addPathToCache( childPath, PathArray, &stbuf, &tmpPathCache );
            }
#endif
        }
        else if ( collEnt.objType == COLL_OBJ_T ) {
            splitPathByKey( collEnt.collName, myDir, mySubDir, '/' );
            filler( buf, mySubDir, NULL, 0 );
#ifdef CACHE_FUSE_PATH
            if ( strcmp( path, "/" ) == 0 ) {
                snprintf( childPath, MAX_NAME_LEN, "/%s", mySubDir );
            }
            else {
                snprintf( childPath, MAX_NAME_LEN, "%s/%s", path, mySubDir );
            }
            if ( matchPathInPathCache( ( char * ) childPath, PathArray,
                                       &tmpPathCache ) != 1 ) {
                fillDirStat( &stbuf,
                             atoi( collEnt.createTime ), atoi( collEnt.modifyTime ),
                             atoi( collEnt.modifyTime ) );
                addPathToCache( childPath, PathArray, &stbuf, &tmpPathCache );
            }
#endif
        }
    }
    rclCloseCollection( &collHandle );
    relIFuseConn( iFuseConn );

    return ( 0 );
}

int
irodsMknod( const char *path, mode_t mode, dev_t rdev ) {
#ifdef CACHE_FUSE_PATH
    int descInx;
    pathCache_t *tmpPathCache = NULL;
#endif
    struct stat stbuf;
    int status = -1;
#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    char cachePath[MAX_NAME_LEN];
#endif
    char irodsPath[MAX_NAME_LEN];
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsMknod: %s", path );


    if ( irodsGetattr( path, &stbuf ) >= 0 ) {
        return -EEXIST;
    }

#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    status = irodsMknodWithCache( ( char * )path, mode, cachePath );
    irodsPath[0] = '\0';
#endif 	/* CACHE_FILE_FOR_NEWLY_CREATED */
    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    if ( status < 0 ) {
        status = dataObjCreateByFusePath( iFuseConn->conn, ( char * ) path,
                                          mode, irodsPath );

        if ( status < 0 ) {
            if ( isReadMsgError( status ) ) {
                ifuseReconnect( iFuseConn );
                status = dataObjCreateByFusePath( iFuseConn->conn,
                                                  ( char * ) path, mode, irodsPath );
            }
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "irodsMknod: rcDataObjCreate of %s error", path );
                relIFuseConn( iFuseConn );
                return -ENOENT;
            }
        }
    }
#ifdef CACHE_FUSE_PATH
    rmPathFromCache( ( char * ) path, NonExistPathArray );
    descInx = allocIFuseDesc();

    if ( descInx < 0 ) {
        rodsLogError( LOG_ERROR, descInx,
                      "irodsMknod: allocIFuseDesc of %s error", path );
        closeIrodsFd( iFuseConn->conn, status );
        relIFuseConn( iFuseConn );
        return 0;
    }
    fillIFuseDesc( descInx, iFuseConn, status, irodsPath,
                   ( char * ) path );
    relIFuseConn( iFuseConn );
    addNewlyCreatedToCache( ( char * ) path, descInx, mode, &tmpPathCache );
    if ( NULL == tmpPathCache ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsMknod :: addNewlyCreatedToCache Failed" );
        return -ENOENT;
    }
#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    tmpPathCache->locCachePath = strdup( cachePath );
    tmpPathCache->locCacheState = HAVE_NEWLY_CREATED_CACHE;
    IFuseDesc[descInx].locCacheState = HAVE_NEWLY_CREATED_CACHE;
    IFuseDesc[descInx].createMode = mode;
#endif

#else   /* CACHE_FUSE_PATH */
    closeIrodsFd( iFuseConn->conn, status );
    relIFuseConn( iFuseConn );
#endif  /* CACHE_FUSE_PATH */

    return ( 0 );
}

int
irodsMkdir( const char *path, mode_t mode ) {
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

    getIFuseConn( &iFuseConn, &MyRodsEnv );

    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcCollCreate( iFuseConn->conn, &collCreateInp );

    if ( status < 0 ) {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcCollCreate( iFuseConn->conn, &collCreateInp );
        }
        relIFuseConn( iFuseConn );
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
        addPathToCache( ( char * ) path, PathArray, &stbuf, NULL );
        rmPathFromCache( ( char * ) path, NonExistPathArray );
#endif
        relIFuseConn( iFuseConn );
    }

    return ( 0 );
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

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcDataObjUnlink( iFuseConn->conn, &dataObjInp );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        rmPathFromCache( ( char * ) path, PathArray );
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
    relIFuseConn( iFuseConn );

    clearKeyVal( &dataObjInp.condInput );

    return ( status );
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

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcRmColl( iFuseConn->conn, &collInp, 0 );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        rmPathFromCache( ( char * ) path, PathArray );
#endif
        status = 0;
    }
    else {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcRmColl( iFuseConn->conn, &collInp, 0 );
        }
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsRmdir: rcRmColl of %s error", path );
            status = -ENOENT;
        }
    }

    relIFuseConn( iFuseConn );

    clearKeyVal( &collInp.condInput );

    return ( status );
}

int
irodsSymlink( const char *from, const char *to ) {
    rodsLog( LOG_DEBUG, "irodsSymlink: %s to %s", from, to );
    return ( 0 );
}

int
irodsRename( const char *from, const char *to ) {
    dataObjCopyInp_t dataObjRenameInp;
    int status;
    pathCache_t *fromPathCache;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsRename: %s to %s", from, to );

#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    if ( matchPathInPathCache( ( char * ) from, PathArray, &fromPathCache ) == 1 &&
            fromPathCache->locCacheState == HAVE_NEWLY_CREATED_CACHE &&
            fromPathCache->locCachePath != NULL ) {
        status = renmeOpenedIFuseDesc( fromPathCache, ( char * ) to );
        if ( status >= 0 ) {
            rmPathFromCache( ( char * ) from, PathArray );
            return ( 0 );
        }
    }
#endif

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

    addKeyVal( &dataObjRenameInp.destDataObjInp.condInput, FORCE_FLAG_KW, "" );

    dataObjRenameInp.srcDataObjInp.oprType =
        dataObjRenameInp.destDataObjInp.oprType = RENAME_UNKNOWN_TYPE;

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcDataObjRename( iFuseConn->conn, &dataObjRenameInp );

    if ( status == CAT_NAME_EXISTS_AS_DATAOBJ ||
            status == SYS_DEST_SPEC_COLL_SUB_EXIST ) {
        rcDataObjUnlink( iFuseConn->conn, &dataObjRenameInp.destDataObjInp );
        status = rcDataObjRename( iFuseConn->conn, &dataObjRenameInp );
    }

    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathCache_t *tmpPathCache;
        rmPathFromCache( ( char * ) to, PathArray );
        if ( matchPathInPathCache( ( char * ) from, PathArray,
                                   &tmpPathCache ) == 1 ) {
            addPathToCache( ( char * ) to, PathArray, &tmpPathCache->stbuf,
                            &tmpPathCache );
            rmPathFromCache( ( char * ) from, PathArray );
        }
        rmPathFromCache( ( char * ) to, NonExistPathArray );
#endif
        status = 0;
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
    relIFuseConn( iFuseConn );

    return ( status );
}

int
irodsLink( const char *from, const char *to ) {
    rodsLog( LOG_DEBUG, "irodsLink: %s to %s" );
    return ( 0 );
}

int
irodsChmod( const char *path, mode_t mode ) {
    int status;
    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;
    char dataMode[SHORT_STR_LEN];
    int descInx;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsChmod: %s", path );

#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    if ( ( descInx = getNewlyCreatedDescByPath( ( char * )path ) ) >= 3 ) {
        /* has not actually been created yet */
        IFuseDesc[descInx].createMode = mode;
        return ( 0 );
    }
#endif
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

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcModDataObjMeta( iFuseConn->conn, &modDataObjMetaInp );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathCache_t *tmpPathCache;

        if ( matchPathInPathCache( ( char * ) path, PathArray,
                                   &tmpPathCache ) == 1 ) {
            tmpPathCache->stbuf.st_mode &= 0xfffffe00;
            tmpPathCache->stbuf.st_mode |= ( mode & 0777 );
        }
#endif
        status = 0;
    }
    else {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcModDataObjMeta( iFuseConn->conn, &modDataObjMetaInp );
        }
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsChmod: rcModDataObjMeta failure" );
            status = -ENOENT;
        }
    }

    relIFuseConn( iFuseConn );
    clearKeyVal( &regParam );

    return( status );
}

int
irodsChown( const char *path, uid_t uid, gid_t gid ) {
    rodsLog( LOG_DEBUG, "irodsChown: %s", path );
    return ( 0 );
}

int
irodsTruncate( const char *path, off_t size ) {
    dataObjInp_t dataObjInp;
    int status;
    pathCache_t *tmpPathCache;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsTruncate: %s", path );

#ifdef CACHE_FILE_FOR_NEWLY_CREATED
    if ( matchPathInPathCache( ( char * ) path, PathArray, &tmpPathCache ) == 1 &&
            tmpPathCache->locCacheState == HAVE_NEWLY_CREATED_CACHE &&
            tmpPathCache->locCachePath != NULL ) {
        status = truncate( tmpPathCache->locCachePath, size );
        if ( status >= 0 ) {
            updatePathCacheStat( tmpPathCache );
            return ( 0 );
        }
    }
#endif

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               dataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsTruncate: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        return -ENOTDIR;
    }

    dataObjInp.dataSize = size;

    getIFuseConn( &iFuseConn, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
    status = rcDataObjTruncate( iFuseConn->conn, &dataObjInp );
    if ( status >= 0 ) {
#ifdef CACHE_FUSE_PATH
        pathCache_t *tmpPathCache;

        if ( matchPathInPathCache( ( char * ) path, PathArray,
                                   &tmpPathCache ) == 1 ) {
            tmpPathCache->stbuf.st_size = size;
        }
#endif
        status = 0;
    }
    else {
        if ( isReadMsgError( status ) ) {
            ifuseReconnect( iFuseConn );
            status = rcDataObjTruncate( iFuseConn->conn, &dataObjInp );
        }
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsTruncate: rcDataObjTruncate of %s error", path );
            status = -ENOENT;
        }
    }
    relIFuseConn( iFuseConn );

    return ( status );
}

int
irodsFlush( const char *path, struct fuse_file_info *fi ) {
    rodsLog( LOG_DEBUG, "irodsFlush: %s", path );
    return ( 0 );
}

int
irodsUtimens( const char *path, const struct timespec ts[2] ) {
    rodsLog( LOG_DEBUG, "irodsUtimens: %s", path );
    return ( 0 );
}

int
irodsOpen( const char *path, struct fuse_file_info *fi ) {
    dataObjInp_t dataObjInp;
    int status;
    int fd;
    int descInx;
    iFuseConn_t *iFuseConn = NULL;

    rodsLog( LOG_DEBUG, "irodsOpen: %s, flags = %d", path, fi->flags );

#ifdef CACHE_FUSE_PATH
    if ( ( descInx = getDescInxInNewlyCreatedCache( ( char * ) path, fi->flags ) )
            > 0 ) {
        rodsLog( LOG_DEBUG, "irodsOpen: a match for %s", path );
        fi->fh = descInx;
        return ( 0 );
    }
#endif
    getIFuseConnByPath( &iFuseConn, ( char * ) path, &MyRodsEnv );
    if ( NULL == iFuseConn ) { // JMC :: cppcheck
        rodsLog( LOG_ERROR, "irodsReaddir :: getIFuseConn Failed" );
        return -ENOENT;
    }
#ifdef CACHE_FILE_FOR_READ
    if ( ( descInx = irodsOpenWithReadCache( iFuseConn,
                     ( char * ) path, fi->flags ) ) > 0 ) {
        rodsLog( LOG_DEBUG, "irodsOpen: a match for %s", path );
        fi->fh = descInx;
        relIFuseConn( iFuseConn );
        return ( 0 );
    }
#endif
    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    status = parseRodsPathStr( ( char * )( path + 1 ) , &MyRodsEnv,
                               dataObjInp.objPath );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "irodsOpen: parseRodsPathStr of %s error", path );
        /* use ENOTDIR for this type of error */
        relIFuseConn( iFuseConn );
        return -ENOTDIR;
    }

    dataObjInp.openFlags = fi->flags;

    fd = rcDataObjOpen( iFuseConn->conn, &dataObjInp );

    if ( fd < 0 ) {
        if ( isReadMsgError( fd ) ) {
            ifuseReconnect( iFuseConn );
            fd = rcDataObjOpen( iFuseConn->conn, &dataObjInp );
        }
        relIFuseConn( iFuseConn );
        if ( fd < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "irodsOpen: rcDataObjOpen of %s error, status = %d", path, fd );
            return -ENOENT;
        }
    }
#ifdef CACHE_FUSE_PATH
    rmPathFromCache( ( char * ) path, NonExistPathArray );
#endif
    descInx = allocIFuseDesc();
    if ( descInx < 0 ) {
        relIFuseConn( iFuseConn );
        rodsLogError( LOG_ERROR, descInx,
                      "irodsOpen: allocIFuseDesc of %s error", path );
        return -ENOENT;
    }
    fillIFuseDesc( descInx, iFuseConn, fd, dataObjInp.objPath,
                   ( char * ) path );
    relIFuseConn( iFuseConn );
    fi->fh = descInx;

    return ( 0 );
}

int
irodsRead( const char *path, char *buf, size_t size, off_t offset,
           struct fuse_file_info *fi ) {
    int descInx;
    int status, myError;

    rodsLog( LOG_DEBUG, "irodsRead: %s", path );

    descInx = fi->fh;

    if ( checkFuseDesc( descInx ) < 0 ) {
        return -EBADF;
    }
    lockDesc( descInx );
    if ( ( status = ifuseLseek( ( char * ) path, descInx, offset ) ) < 0 ) {
        unlockDesc( descInx );
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return ( -myError );
        }
        else {
            return -ENOENT;
        }
    }

    if ( size <= 0 ) {
        unlockDesc( descInx );
        return 0;
    }

    status = ifuseRead( ( char * ) path, descInx, buf, size, offset );

    unlockDesc( descInx );

    return status;
}

int
irodsWrite( const char *path, const char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi ) {
    int descInx;
    int status, myError;

    rodsLog( LOG_DEBUG, "irodsWrite: %s", path );

    descInx = fi->fh;

    if ( checkFuseDesc( descInx ) < 0 ) {
        return -EBADF;
    }

    lockDesc( descInx );
    if ( ( status = ifuseLseek( ( char * ) path, descInx, offset ) ) < 0 ) {
        unlockDesc( descInx );
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return ( -myError );
        }
        else {
            return -ENOENT;
        }
    }

    if ( size <= 0 ) {
        unlockDesc( descInx );
        return 0;
    }

    status = ifuseWrite( ( char * ) path, descInx, ( char * )buf, size, offset );
    unlockDesc( descInx );

    return status;
}

int
irodsStatfs( const char *path, struct statvfs *stbuf ) {

    rodsLog( LOG_DEBUG, "irodsStatfs: %s", path );

    if ( stbuf == NULL ) {
        return ( 0 );
    }


    /* just fake some number */
    if ( !statvfs( "/", stbuf ) ) {}

    stbuf->f_bsize = FILE_BLOCK_SZ;
    stbuf->f_blocks = 2000000000;
    stbuf->f_bfree = stbuf->f_bavail = 1000000000;
    stbuf->f_files = 200000000;
    stbuf->f_ffree = stbuf->f_favail = 100000000;
    stbuf->f_fsid = 777;
    stbuf->f_namemax = MAX_NAME_LEN;

    return ( 0 );
}

int
irodsRelease( const char *path, struct fuse_file_info *fi ) {
    int descInx;
    int status, myError;

    rodsLog( LOG_DEBUG, "irodsRelease: %s", path );

    descInx = fi->fh;

    if ( checkFuseDesc( descInx ) < 0 ) {
        return -EBADF;
    }

    status = ifuseClose( ( char * ) path, descInx );

    freeIFuseDesc( descInx );

    if ( status < 0 ) {
        if ( ( myError = getErrno( status ) ) > 0 ) {
            return ( -myError );
        }
        else {
            return -ENOENT;
        }
    }
    else {
        return ( 0 );
    }
}

int
irodsFsync( const char *path, int isdatasync, struct fuse_file_info *fi ) {
    rodsLog( LOG_DEBUG, "irodsFsync: %s", path );
    return ( 0 );
}

