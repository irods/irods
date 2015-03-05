/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsDataObjRename.c - rename a data object.
 */
#include "dataObjRename.hpp"
#include "reFuncDefs.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "collection.hpp"
#include "resource.hpp"
#include "specColl.hpp"
#include "physPath.hpp"
#include "subStructFileRename.hpp"
#include "icatHighLevelRoutines.hpp"
#include "dataObjUnlink.hpp"
#include "phyBundleColl.hpp"
#include "regDataObj.hpp"
#include "fileOpendir.hpp"
#include "fileReaddir.hpp"
#include "fileClosedir.hpp"
#include "rmColl.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"

int
rsDataObjRename( rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp ) {
    int status;
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *srcDataObjInfo = NULL;
    dataObjInfo_t *destDataObjInfo = NULL;
    specCollCache_t *specCollCache = NULL;
    int srcType, destType;

    srcDataObjInp = &dataObjRenameInp->srcDataObjInp;
    destDataObjInp = &dataObjRenameInp->destDataObjInp;

    /* don't translate the link pt. treat it as a normal collection */
    addKeyVal( &srcDataObjInp->condInput, NO_TRANSLATE_LINKPT_KW, "" );
    resolveLinkedPath( rsComm, srcDataObjInp->objPath, &specCollCache,
                       &srcDataObjInp->condInput );
    rmKeyVal( &srcDataObjInp->condInput, NO_TRANSLATE_LINKPT_KW );

    resolveLinkedPath( rsComm, destDataObjInp->objPath, &specCollCache,
                       &destDataObjInp->condInput );

    if ( strcmp( srcDataObjInp->objPath, destDataObjInp->objPath ) == 0 ) {
        return SAME_SRC_DEST_PATHS_ERR;
    }

    /* connect to rcat for cross zone */
    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )srcDataObjInp->objPath,
                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) {
        return status;
    }
    else if ( rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
        status = rcDataObjRename( rodsServerHost->conn, dataObjRenameInp );
        return status;
    }

    srcType = resolvePathInSpecColl( rsComm, srcDataObjInp->objPath,
                                     WRITE_COLL_PERM, 0, &srcDataObjInfo );

    destType = resolvePathInSpecColl( rsComm, destDataObjInp->objPath,
                                      WRITE_COLL_PERM, 0, &destDataObjInfo );

    if ( srcDataObjInfo           != NULL &&
            srcDataObjInfo->specColl != NULL &&
            strcmp( srcDataObjInfo->specColl->collection,
                    srcDataObjInp->objPath ) == 0 ) {
        /* this must be the link pt or mount pt. treat it as normal coll */
        freeDataObjInfo( srcDataObjInfo );
        srcDataObjInfo = NULL;
        srcType = SYS_SPEC_COLL_NOT_IN_CACHE;
    }

    if ( !isSameZone( srcDataObjInp->objPath, destDataObjInp->objPath ) ) {
        return SYS_CROSS_ZONE_MV_NOT_SUPPORTED;
    }

    if ( destType >= 0 ) {
        rodsLog( LOG_ERROR,
                 "rsDataObjRename: dest specColl objPath %s exists",
                 destDataObjInp->objPath );
        freeDataObjInfo( srcDataObjInfo );
        freeDataObjInfo( destDataObjInfo );
        return SYS_DEST_SPEC_COLL_SUB_EXIST;
    }

    if ( srcType >= 0 ) { /* specColl of some sort */
        if ( srcDataObjInfo && destType == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            status = specCollObjRename( rsComm, srcDataObjInfo,
                                        destDataObjInfo );
        }
        else {
            /* dest is regular obj. Allow special case where the src
             * is in a MOUNTED_COLL */
            if ( srcDataObjInfo && srcDataObjInfo->specColl->collClass == MOUNTED_COLL ) {
                /* a special case for moving obj from mounted collection to
                 * regular collection */
                status = moveMountedCollObj( rsComm, srcDataObjInfo, srcType,
                                             destDataObjInp );
            }
            else {
                rodsLog( LOG_ERROR,
                         "rsDataObjRename: src %s is in spec coll but dest %s is not",
                         srcDataObjInp->objPath, destDataObjInp->objPath );
                status = SYS_SRC_DEST_SPEC_COLL_CONFLICT;
            }
        }
        freeDataObjInfo( srcDataObjInfo );
        freeDataObjInfo( destDataObjInfo );
        return status;
    }
    else if ( srcType == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
        return SYS_SPEC_COLL_OBJ_NOT_EXIST;
    }
    else if ( destType == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
        /* source is normal object but dest is not */
        rodsLog( LOG_ERROR,
                 "rsDataObjRename: src %s is not in spec coll but dest %s is",
                 srcDataObjInp->objPath, destDataObjInp->objPath );
        return SYS_SRC_DEST_SPEC_COLL_CONFLICT;
    }

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )dataObjRenameInp->srcDataObjInp.objPath,
                 &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsDataObjRename( rsComm, dataObjRenameInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcDataObjRename( rodsServerHost->conn, dataObjRenameInp );
    }

    return status;
}

int
getMultiCopyPerResc( rsComm_t *rsComm ) { // JMC - backport 4556
    ruleExecInfo_t rei;

    memset( &rei, 0, sizeof( rei ) );
    rei.rsComm = rsComm; // JMC - backport 4556
    applyRule( "acSetMultiReplPerResc", NULL, &rei, NO_SAVE_REI );
    if ( strcmp( rei.statusStr, MULTI_COPIES_PER_RESC ) == 0 ) {
        return 1;
    }
    else {
        return 0;
    }
}


int
_rsDataObjRename( rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp ) {
#ifdef RODS_CAT
    if ( rsComm == NULL ) {
        rodsLog( LOG_ERROR, "_rsDataObjRename was passed a null rsComm" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    int status;
    char srcColl[MAX_NAME_LEN], srcObj[MAX_NAME_LEN];
    char destColl[MAX_NAME_LEN], destObj[MAX_NAME_LEN];
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    dataObjInfo_t *dataObjInfoHead = NULL;
    rodsLong_t srcId, destId;
    int multiCopyFlag;
    int acPreProcFromRenameFlag = 0;

    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    int i, argc;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    rei2.uoic = &rsComm->clientUser;
    rei2.uoip = &rsComm->proxyUser;
    rei2.doinp = &dataObjRenameInp->srcDataObjInp;

    srcDataObjInp = &dataObjRenameInp->srcDataObjInp;
    destDataObjInp = &dataObjRenameInp->destDataObjInp;

    if ( ( status = splitPathByKey(
                        srcDataObjInp->objPath, srcColl, MAX_NAME_LEN, srcObj, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsDataObjRename: splitPathByKey for %s error, status = %d",
                 srcDataObjInp->objPath, status );
        return status;
    }

    if ( ( status = splitPathByKey(
                        destDataObjInp->objPath, destColl, MAX_NAME_LEN, destObj, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsDataObjRename: splitPathByKey for %s error, status = %d",
                 destDataObjInp->objPath, status );
        return status;
    }

    multiCopyFlag = getMultiCopyPerResc( rsComm );  // JMC - backport 4556

    if ( srcDataObjInp->oprType == RENAME_DATA_OBJ ) {

        status = getDataObjInfo( rsComm, srcDataObjInp, &dataObjInfoHead, ACCESS_DELETE_OBJECT, 0 );

        if ( status >= 0 || NULL != dataObjInfoHead ) {
            srcId = dataObjInfoHead->dataId;
        }
        else {
            rodsLog( LOG_ERROR,
                     "_rsDataObjRename: src data %s does not exist, status = %d",
                     srcDataObjInp->objPath, status );
            return status;
        }
    }
    else if ( srcDataObjInp->oprType == RENAME_COLL ) {
        status = isColl( rsComm, srcDataObjInp->objPath, &srcId );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsDataObjRename: src coll %s does not exist, status = %d",
                     srcDataObjInp->objPath, status );
            return status;
        }
    }
    else {
        if ( ( status = isData( rsComm, srcDataObjInp->objPath, &srcId ) ) >= 0 ) {
            if ( isData( rsComm, destDataObjInp->objPath, &destId ) >= 0 &&
                    getValByKey( &srcDataObjInp->condInput, FORCE_FLAG_KW ) != NULL ) {
                /* dest exist */
                rsDataObjUnlink( rsComm, destDataObjInp );
            }
            srcDataObjInp->oprType = destDataObjInp->oprType = RENAME_DATA_OBJ;
            status = getDataObjInfo( rsComm, srcDataObjInp, &dataObjInfoHead,
                                     ACCESS_DELETE_OBJECT, 0 );

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsDataObjRename: src data %s does not exist, status = %d",
                         srcDataObjInp->objPath, status );
                return status;
            }
        }
        else if ( ( status = isColl( rsComm, srcDataObjInp->objPath, &srcId ) )
                  >= 0 ) {
            srcDataObjInp->oprType = destDataObjInp->oprType = RENAME_COLL;
        }
        else {
            rodsLog( LOG_ERROR,
                     "_rsDataObjRename: src obj %s does not exist, status = %d",
                     srcDataObjInp->objPath, status );
            return status;
        }
    }

    if ( srcDataObjInp->oprType == RENAME_DATA_OBJ ) {
        if ( strstr( dataObjInfoHead->dataType, BUNDLE_STR ) != NULL ) { // JMC - backport 4658
            rodsLog( LOG_ERROR,
                     "_rsDataObjRename: cannot rename tar bundle type obj %s",
                     srcDataObjInp->objPath );
            return CANT_RM_MV_BUNDLE_TYPE;
        }
    }

    if ( strcmp( srcObj, destObj ) != 0 ) {
        /* rename */
        if ( srcId < 0 ) {
            status = srcId;
            return status;
        }

        /**  June 1 2009 for pre-post processing rule hooks **/
        argc    = 2;
        args[0] = srcDataObjInp->objPath;
        args[1] = destDataObjInp->objPath;
        acPreProcFromRenameFlag = 1;
        i =  applyRuleArg( "acPreProcForObjRename", args, argc, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            if ( rei2.status < 0 ) {
                i = rei2.status;
            }
            rodsLog( LOG_ERROR,
                     "rsDataObjRename: acPreProcForObjRename error for source %s and destination %s,stat=%d",
                     args[0], args[1], i );
            return i;
        }
        /**  June 1 2009 for pre-post processing rule hooks **/

        status = chlRenameObject( rsComm, srcId, destObj );
    }

    if ( status < 0 ) {
        return status;
    }

    if ( strcmp( srcColl, destColl ) != 0 ) {
        /* move. The destColl is the target  */
        status = isColl( rsComm, destColl, &destId );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsDataObjRename: dest coll %s does not exist, status = %d",
                     destColl, status );
            return status;
        }

        /**  June 1 2009 for pre-post processing rule hooks **/
        if ( acPreProcFromRenameFlag == 0 ) {
            args[0] = srcDataObjInp->objPath;
            args[1] = destDataObjInp->objPath;
            argc = 2;
            i =  applyRuleArg( "acPreProcForObjRename", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsDataObjRename: acPreProcForObjRename error for source %s and destination %s,stat=%d",
                         args[0], args[1], i );
                return i;
            }
        }
        /**  June 1 2009 for pre-post processing rule hooks **/

        status = chlMoveObject( rsComm, srcId, destId );
    }
    if ( status >= 0 ) {
        if ( multiCopyFlag > 0 ) {
            status = chlCommit( rsComm );
            return status;
        }
        else {
            /* enforce physPath consistency */
            if ( srcDataObjInp->oprType == RENAME_DATA_OBJ ) {
                dataObjInfo_t *tmpDataObjInfo;

                /* update src dataObjInfoHead with dest objPath */
                tmpDataObjInfo = dataObjInfoHead;
                while ( tmpDataObjInfo != NULL ) {
                    rstrcpy( tmpDataObjInfo->objPath, destDataObjInp->objPath, MAX_NAME_LEN );

                    tmpDataObjInfo = tmpDataObjInfo->next;
                }
                status = syncDataObjPhyPath( rsComm, destDataObjInp, dataObjInfoHead, NULL );
                freeAllDataObjInfo( dataObjInfoHead );
            }
            else {
                status = syncCollPhyPath( rsComm, destDataObjInp->objPath );
            }

            if ( status >= 0 ) {
                status = chlCommit( rsComm );
            }
            else {
                chlRollback( rsComm );
            }
        }
        if ( status >= 0 ) {
            args[0] = srcDataObjInp->objPath;
            args[1] = destDataObjInp->objPath;
            argc = 2;
            status =  applyRuleArg( "acPostProcForObjRename", args, argc,
                                    &rei2, NO_SAVE_REI );
            if ( status < 0 ) {
                if ( rei2.status < 0 ) {
                    status = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsDataObjRename: acPostProc err for src %s dest %s,stat=%d",
                         args[0], args[1], status );
            }
        }
    }
    else {
        chlRollback( rsComm );
    }
    return status;
#else
    return SYS_NO_ICAT_SERVER_ERR;
#endif
}

int
specCollObjRename( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                   dataObjInfo_t *destDataObjInfo ) {
    int status;
    char *newPath;

    if ( getStructFileType( srcDataObjInfo->specColl ) >= 0 ) {
        /* structFile type, use the logical subPath */
        newPath = destDataObjInfo->subPath;
    }
    else {
        /* mounted fs, use phy path */
        newPath = destDataObjInfo->filePath;
    }

    status = l3Rename( rsComm, srcDataObjInfo, newPath );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "specCollObjRename: l3Rename error from %s to %s, status = %d",
                 srcDataObjInfo->subPath, newPath, status );
        return status;
    }
    return status;
}

int
l3Rename( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *newFileName ) {
    fileRenameInp_t fileRenameInp;
    int status;

    irods::error resc_err = irods::is_hier_live( dataObjInfo->rescHier );
    if ( !resc_err.ok() ) {
        return resc_err.code();
    }

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Rename - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileRenameInp_t subStructFileRenameInp;
        memset( &subStructFileRenameInp, 0, sizeof( subStructFileRenameInp ) );
        rstrcpy( subStructFileRenameInp.subFile.subFilePath, dataObjInfo->subPath,
                 MAX_NAME_LEN );
        rstrcpy( subStructFileRenameInp.newSubFilePath, newFileName, MAX_NAME_LEN );
        rstrcpy( subStructFileRenameInp.subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileRenameInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        subStructFileRenameInp.subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileRename( rsComm, &subStructFileRenameInp );
    }
    else {
        memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
        rstrcpy( fileRenameInp.oldFileName,   dataObjInfo->filePath,  MAX_NAME_LEN );
        rstrcpy( fileRenameInp.newFileName,   newFileName,            MAX_NAME_LEN );
        rstrcpy( fileRenameInp.rescHier,      dataObjInfo->rescHier,  MAX_NAME_LEN );
        rstrcpy( fileRenameInp.objPath,       dataObjInfo->objPath,   MAX_NAME_LEN );
        rstrcpy( fileRenameInp.addr.hostAddr, location.c_str(),       NAME_LEN );
        fileRenameOut_t* ren_out = 0;
        status = rsFileRename( rsComm, &fileRenameInp, &ren_out );
        free( ren_out );
    }
    return status;
}

/* moveMountedCollObj - move a mounted collecion obj to a normal obj */
int
moveMountedCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                    int srcType, dataObjInp_t *destDataObjInp ) {
    int status;

    if ( srcType == DATA_OBJ_T ) {
        status = moveMountedCollDataObj( rsComm, srcDataObjInfo,
                                         destDataObjInp );
    }
    else if ( srcType == COLL_OBJ_T ) {
        status = moveMountedCollCollObj( rsComm, srcDataObjInfo,
                                         destDataObjInp );
    }
    else {
        status = SYS_UNMATCHED_SPEC_COLL_TYPE;
    }
    return status;
}

int
moveMountedCollDataObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                        dataObjInp_t *destDataObjInp ) {
    dataObjInfo_t destDataObjInfo;
    fileRenameInp_t fileRenameInp;
    int status;

    if ( rsComm == NULL || srcDataObjInfo == NULL || destDataObjInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    bzero( &destDataObjInfo, sizeof( destDataObjInfo ) );
    bzero( &fileRenameInp, sizeof( fileRenameInp ) );
    rstrcpy( destDataObjInfo.objPath, destDataObjInp->objPath, MAX_NAME_LEN );
    rstrcpy( destDataObjInfo.dataType, srcDataObjInfo->dataType, NAME_LEN );
    destDataObjInfo.dataSize = srcDataObjInfo->dataSize;

    rstrcpy( destDataObjInfo.rescName, srcDataObjInfo->rescName, NAME_LEN );
    rstrcpy( destDataObjInfo.rescHier, srcDataObjInfo->rescHier, MAX_NAME_LEN );
    status = getFilePathName( rsComm, &destDataObjInfo, destDataObjInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: getFilePathName err for %s. status = %d",
                 destDataObjInfo.objPath, status );
        return status;
    }

    status = filePathTypeInResc( rsComm, destDataObjInfo.objPath, destDataObjInfo.filePath, destDataObjInfo.rescHier );
    if ( status == LOCAL_DIR_T ) {
        status = SYS_PATH_IS_NOT_A_FILE;
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: targ path %s is a dir. status = %d",
                 destDataObjInfo.filePath, status );
        return status;
    }
    else if ( status == LOCAL_FILE_T ) {
        dataObjInfo_t myDataObjInfo;
        status = chkOrphanFile( rsComm, destDataObjInfo.filePath,
                                destDataObjInfo.rescName, &myDataObjInfo );
        if ( status == 1 ) {
            /* orphan */
            char new_fn[ MAX_NAME_LEN ];
            rstrcpy( fileRenameInp.oldFileName, destDataObjInfo.filePath, MAX_NAME_LEN );
            int error_code = renameFilePathToNewDir( rsComm, ORPHAN_DIR, &fileRenameInp, 1, new_fn );
            if ( error_code < 0 ) {
                rodsLog( LOG_ERROR, "renameFilePathToNewDir failed in moveMountedCollDataObj with error code %d", error_code );
            }
            snprintf( destDataObjInfo.filePath, sizeof( destDataObjInfo.filePath ), "%s", new_fn );
        }
        else if ( status == 0 ) {
            /* obj exist */
            return SYS_COPY_ALREADY_IN_RESC;
        }
        else {
            return status;
        }
    }
    status = l3Rename( rsComm, srcDataObjInfo, destDataObjInfo.filePath );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: l3Rename error from %s to %s, status = %d",
                 srcDataObjInfo->filePath, destDataObjInfo.filePath, status );
        return status;
    }
    status = svrRegDataObj( rsComm, &destDataObjInfo );
    if ( status < 0 ) {
        l3Rename( rsComm, &destDataObjInfo, srcDataObjInfo->filePath );
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: rsRegDataObj for %s failed, status = %d",
                 destDataObjInfo.objPath, status );
    }
    return status;
}

int
moveMountedCollCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                        dataObjInp_t *destDataObjInp ) {
    int l3descInx;
    fileReaddirInp_t fileReaddirInp;
    rodsDirent_t *rodsDirent = NULL;
    rodsStat_t *fileStatOut = NULL;
    dataObjInfo_t subSrcDataObjInfo;
    dataObjInp_t subDestDataObjInp;
    int status;
    int savedStatus = 0;

    subSrcDataObjInfo = *srcDataObjInfo;
    bzero( &subDestDataObjInp, sizeof( subDestDataObjInp ) );
    l3descInx = l3Opendir( rsComm, srcDataObjInfo );
    fileReaddirInp.fileInx = l3descInx;
    rsMkCollR( rsComm, "/", destDataObjInp->objPath );
    while ( rsFileReaddir( rsComm, &fileReaddirInp, &rodsDirent ) >= 0 ) {
        rodsDirent_t myRodsDirent;

        myRodsDirent = *rodsDirent;
        free( rodsDirent );

        if ( strcmp( myRodsDirent.d_name, "." ) == 0 ||
                strcmp( myRodsDirent.d_name, ".." ) == 0 ) {
            continue;
        }

        snprintf( subSrcDataObjInfo.objPath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->objPath, myRodsDirent.d_name );
        snprintf( subSrcDataObjInfo.subPath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->subPath, myRodsDirent.d_name );
        snprintf( subSrcDataObjInfo.filePath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->filePath, myRodsDirent.d_name );

        status = l3Stat( rsComm, &subSrcDataObjInfo, &fileStatOut );
        if ( status < 0 || fileStatOut == NULL ) { // JMC cppcheck
            rodsLog( LOG_ERROR,
                     "moveMountedCollCollObj: l3Stat for %s error, status = %d",
                     subSrcDataObjInfo.filePath, status );
            return status;
        }
        snprintf( subSrcDataObjInfo.dataCreate, TIME_LEN, "%d",
                  fileStatOut->st_ctim );
        snprintf( subSrcDataObjInfo.dataModify, TIME_LEN, "%d",
                  fileStatOut->st_mtim );
        snprintf( subDestDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
                  destDataObjInp->objPath, myRodsDirent.d_name );
        if ( ( fileStatOut->st_mode & S_IFREG ) != 0 ) { /* a file */
            subSrcDataObjInfo.dataSize = fileStatOut->st_size;
            status = moveMountedCollDataObj( rsComm, &subSrcDataObjInfo,
                                             &subDestDataObjInp );
        }
        else {
            status = moveMountedCollCollObj( rsComm, &subSrcDataObjInfo,
                                             &subDestDataObjInp );
        }
        if ( status < 0 ) {
            savedStatus = status;
            rodsLog( LOG_ERROR,
                     "moveMountedCollCollObj: moveMountedColl for %s error, stat = %d",
                     subSrcDataObjInfo.objPath, status );
        }
        //if (fileStatOut != NULL) {
        free( fileStatOut );
        fileStatOut = NULL;
        //}
    }
    l3Rmdir( rsComm, srcDataObjInfo );
    return savedStatus;
}

