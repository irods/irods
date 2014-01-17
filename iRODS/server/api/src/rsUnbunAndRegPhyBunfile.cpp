/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsStructFileExtAndReg.c. See unbunAndRegPhyBunfile.h for a description of
 * this API call.*/

#include "unbunAndRegPhyBunfile.hpp"
#include "apiHeaderAll.hpp"
#include "miscServerFunct.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"

#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"

int
rsUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    char *rescName;

    if ( ( rescName = getValByKey( &dataObjInp->condInput, DEST_RESC_NAME_KW ) )
            == NULL ) {
        return USER_NO_RESC_INPUT_ERR;
    }

    rescInfo_t* rescInfo = new rescInfo_t;
    irods::error err = irods::get_resc_info( rescName, *rescInfo );
    if ( !err.ok() ) {
        delete rescInfo;
        std::stringstream msg;
        msg << "failed for [";
        msg << rescName;
        msg << "]";
        irods::log( PASSMSG( msg.str(), err ) );
        return -1;
    }
    status = _rsUnbunAndRegPhyBunfile( rsComm, dataObjInp, rescInfo );

    return ( status );
}

int
_rsUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                          rescInfo_t *rescInfo ) {
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    char *bunFilePath;
    char phyBunDir[MAX_NAME_LEN];
    int rmBunCopyFlag;
    char *dataType = NULL; // JMC - backport 4664

    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( !resc_hier ) {
        rodsLog( LOG_NOTICE, "_rsUnbunAndRegPhyBunfile - RESC_HIER_STR_KW is NULL" );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return -1;
    }

    //remoteFlag = resolveHostByRescInfo (rescInfo, &rodsServerHost);
    rodsHostAddr_t addr;
    memset( &addr, 0, sizeof( addr ) );
    rstrcpy( addr.hostAddr, location.c_str(), NAME_LEN );
    remoteFlag = resolveHost( &addr, &rodsServerHost );
    if ( remoteFlag == REMOTE_HOST ) {
        addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW,
                   rescInfo->rescName );
        status = remoteUnbunAndRegPhyBunfile( rsComm, dataObjInp,
                                              rodsServerHost );
        return status;
    }
    /* process this locally */
    if ( ( bunFilePath = getValByKey( &dataObjInp->condInput, BUN_FILE_PATH_KW ) ) // JMC - backport 4768
            == NULL ) {
        rodsLog( LOG_ERROR,
                 "_rsUnbunAndRegPhyBunfile: No filePath input for %s",
                 dataObjInp->objPath );
        return ( SYS_INVALID_FILE_PATH );
    }

    createPhyBundleDir( rsComm, bunFilePath, phyBunDir );
    dataType = getValByKey( &dataObjInp->condInput, DATA_TYPE_KW ); // JMC - backport 4664
    status = unbunPhyBunFile( rsComm, dataObjInp->objPath, rescInfo, bunFilePath, phyBunDir,
                              dataType, 0, resc_hier ); // JMC - backport 4632, 4657, 4664

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsUnbunAndRegPhyBunfile:unbunPhyBunFile err for %s to dir %s.stat=%d",
                 bunFilePath, phyBunDir, status );
        return status;
    }

    if ( getValByKey( &dataObjInp->condInput, RM_BUN_COPY_KW ) == NULL ) {
        rmBunCopyFlag = 0;
    }
    else {
        rmBunCopyFlag = 1;
    }

    status = regUnbunPhySubfiles( rsComm, rescInfo, phyBunDir, rmBunCopyFlag );

    if ( status == CAT_NO_ROWS_FOUND ) {
        /* some subfiles have been deleted. harmless */
        status = 0;
    }
    else if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsUnbunAndRegPhyBunfile: regUnbunPhySubfiles for dir %s. stat = %d",
                 phyBunDir, status );
    }

    return status;
}

int
regUnbunPhySubfiles( rsComm_t *rsComm, rescInfo_t *rescInfo, char *phyBunDir,
                     int rmBunCopyFlag ) {
    char subfilePath[MAX_NAME_LEN];
    dataObjInp_t dataObjInp;
    dataObjInp_t dataObjUnlinkInp;
    int status = 0;
    int savedStatus = 0;

    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *bunDataObjInfo = NULL;       /* the copy in BUNDLE_RESC */
    path srcDirPath( phyBunDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "regUnbunphySubfiles: opendir error for %s, errno = %d",
                 phyBunDir, errno );
        return ( UNIX_FILE_OPENDIR_ERR - errno );
    }
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    if ( rmBunCopyFlag > 0 ) {
        bzero( &dataObjUnlinkInp, sizeof( dataObjUnlinkInp ) );
        addKeyVal( &dataObjUnlinkInp.condInput, ADMIN_KW, "" );
    }

    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( subfilePath, MAX_NAME_LEN, "%s",
                  p.c_str() );

        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "regUnbunphySubfiles: stat error for %s, errno = %d",
                     subfilePath, errno );
            return ( UNIX_FILE_STAT_ERR - errno );
        }

        if ( !is_regular_file( p ) ) {
            continue;
        }

        path childPath = p.filename();
        /* do the registration */
        addKeyVal( &dataObjInp.condInput, QUERY_BY_DATA_ID_KW,
                   ( char * ) childPath.c_str() );
        status = getDataObjInfo( rsComm, &dataObjInp, &dataObjInfoHead,
                                 NULL, 1 );
        if ( status < 0 ) {
            rodsLog( LOG_DEBUG,
                     "regUnbunphySubfiles: getDataObjInfo error for %s, status = %d",
                     subfilePath, status );
            /* don't terminate beause the data Obj may be deleted */
            unlink( subfilePath );
            continue;
        }
        requeDataObjInfoByResc( &dataObjInfoHead, BUNDLE_RESC, 1, 1 );
        bunDataObjInfo = NULL;
        if ( strcmp( dataObjInfoHead->rescName, BUNDLE_RESC ) != 0 ) {
            /* no match */
            rodsLog( LOG_DEBUG,
                     "regUnbunphySubfiles: No copy in BUNDLE_RESC for %s",
                     dataObjInfoHead->objPath );
            /* don't terminate beause the copy may be deleted */
            unlink( subfilePath );
            continue;
        }
        else {
            bunDataObjInfo = dataObjInfoHead;
        }
        requeDataObjInfoByResc( &dataObjInfoHead, rescInfo->rescName, 1, 1 );
        /* The copy in DEST_RESC_NAME_KW should be on top */
        if ( strcmp( dataObjInfoHead->rescName, rescInfo->rescName ) != 0 ) {
            /* no copy. stage it */
            status = regPhySubFile( rsComm, subfilePath, bunDataObjInfo,
                                    rescInfo );
            unlink( subfilePath );
            if ( status < 0 ) {
                rodsLog( LOG_DEBUG,
                         "regUnbunphySubfiles: regPhySubFile err for %s, status = %d",
                         bunDataObjInfo->objPath, status );
            }
        }
        else {
            /* XXXXXX have a copy. don't do anything for now */
            unlink( subfilePath );
        }
        if ( rmBunCopyFlag > 0 ) {
            rstrcpy( dataObjUnlinkInp.objPath, bunDataObjInfo->objPath,
                     MAX_NAME_LEN );
            status = dataObjUnlinkS( rsComm, &dataObjUnlinkInp, bunDataObjInfo );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "regUnbunphySubfiles: dataObjUnlinkS err for %s, status = %d",
                         bunDataObjInfo->objPath, status );
                savedStatus = status;
            }
        }
        freeAllDataObjInfo( dataObjInfoHead );

    }
    clearKeyVal( &dataObjInp.condInput );
    if ( status >= 0 && savedStatus < 0 ) {
        return savedStatus;
    }
    else {
        return status;
    }
}

int
regPhySubFile( rsComm_t *rsComm, char *subfilePath,
               dataObjInfo_t *bunDataObjInfo, rescInfo_t *rescInfo ) {
    dataObjInfo_t stageDataObjInfo;
    dataObjInp_t dataObjInp;
    int status;
    regReplica_t regReplicaInp;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    bzero( &stageDataObjInfo, sizeof( stageDataObjInfo ) );
    rstrcpy( dataObjInp.objPath, bunDataObjInfo->objPath, MAX_NAME_LEN );
    rstrcpy( stageDataObjInfo.objPath, bunDataObjInfo->objPath, MAX_NAME_LEN );
    rstrcpy( stageDataObjInfo.rescName, rescInfo->rescName, NAME_LEN );
    stageDataObjInfo.rescInfo = new rescInfo_t;
    memcpy( stageDataObjInfo.rescInfo, rescInfo, sizeof( rescInfo_t ) );

    status = getFilePathName( rsComm, &stageDataObjInfo, &dataObjInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regPhySubFile: getFilePathName err for %s. status = %d",
                 dataObjInp.objPath, status );
        delete stageDataObjInfo.rescInfo;
        return ( status );
    }

    path p( stageDataObjInfo.filePath );
    if ( exists( p ) ) {
        status = resolveDupFilePath( rsComm, &stageDataObjInfo, &dataObjInp );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "regPhySubFile: resolveDupFilePath err for %s. status = %d",
                     stageDataObjInfo.filePath, status );
            delete stageDataObjInfo.rescInfo;
            return ( status );
        }
    }

    /* make the necessary dir */
    mkDirForFilePath(
        rsComm,
        "/",
        stageDataObjInfo.filePath,
        stageDataObjInfo.rescHier,
        getDefDirMode() );

    /* add a link */
    status = link( subfilePath, stageDataObjInfo.filePath );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regPhySubFile: link error %s to %s. errno = %d",
                 subfilePath, stageDataObjInfo.filePath, errno );
        delete stageDataObjInfo.rescInfo;
        return ( UNIX_FILE_LINK_ERR - errno );
    }

    bzero( &regReplicaInp, sizeof( regReplicaInp ) );
    regReplicaInp.srcDataObjInfo = bunDataObjInfo;
    regReplicaInp.destDataObjInfo = &stageDataObjInfo;
    addKeyVal( &regReplicaInp.condInput, SU_CLIENT_USER_KW, "" );
    addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );

    status = rsRegReplica( rsComm, &regReplicaInp );

    clearKeyVal( &regReplicaInp.condInput );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regPhySubFile: rsRegReplica error for %s. status = %d",
                 bunDataObjInfo->objPath, status );
        delete stageDataObjInfo.rescInfo;
        return status;
    }

    delete stageDataObjInfo.rescInfo;
    return status;
}

int unbunPhyBunFile( rsComm_t *rsComm, char *objPath,
                     rescInfo_t *rescInfo, char *bunFilePath, char *phyBunDir,
                     char *dataType, int oprType, const char* resc_hier ) { // JMC - backport 4632, 4657
    int status;
    structFileOprInp_t structFileOprInp;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return -1;
    }

    /* untar the bunfile */
    memset( &structFileOprInp, 0, sizeof( structFileOprInp_t ) );
    structFileOprInp.specColl = ( specColl_t* )malloc( sizeof( specColl_t ) );
    memset( structFileOprInp.specColl, 0, sizeof( specColl_t ) );
    structFileOprInp.specColl->type = TAR_STRUCT_FILE_T;
    snprintf( structFileOprInp.specColl->collection, MAX_NAME_LEN,
              "%s.dir", objPath ); // JMC - backport 4657
    rstrcpy( structFileOprInp.specColl->objPath,
             objPath, MAX_NAME_LEN ); // JMC - backport 4657
    structFileOprInp.specColl->collClass = STRUCT_FILE_COLL;
    rstrcpy( structFileOprInp.specColl->resource, rescInfo->rescName, NAME_LEN );
    rstrcpy( structFileOprInp.specColl->phyPath, bunFilePath, MAX_NAME_LEN );
    rstrcpy( structFileOprInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( structFileOprInp.specColl->rescHier, resc_hier, MAX_NAME_LEN );

    /* set the cacheDir */
    rstrcpy( structFileOprInp.specColl->cacheDir, phyBunDir, MAX_NAME_LEN );
    /* pass on the dataType */


    if ( dataType != NULL && // JMC - backport 4632
            ( strstr( dataType, GZIP_TAR_DT_STR )  != NULL || // JMC - backport 4658
              strstr( dataType, BZIP2_TAR_DT_STR ) != NULL ||
              strstr( dataType, ZIP_DT_STR )       != NULL ) ) {
        addKeyVal( &structFileOprInp.condInput, DATA_TYPE_KW, dataType );
    }

    if ( ( oprType & PRESERVE_DIR_CONT ) == 0 ) { // JMC - backport 4657
        rmLinkedFilesInUnixDir( phyBunDir );
    }
    structFileOprInp.oprType = oprType; // JMC - backport 4657
    status = rsStructFileExtract( rsComm, &structFileOprInp );
    if ( status == SYS_DIR_IN_VAULT_NOT_EMPTY ) {
        /* phyBunDir is not empty */
        if ( chkOrphanDir( rsComm, phyBunDir, rescInfo->rescName ) > 0 ) {
            /* it is a orphan dir */
            fileRenameInp_t fileRenameInp;
            bzero( &fileRenameInp, sizeof( fileRenameInp ) );
            rstrcpy( fileRenameInp.oldFileName, phyBunDir, MAX_NAME_LEN );
            char new_fn[ MAX_NAME_LEN ];
            status = renameFilePathToNewDir( rsComm, ORPHAN_DIR,
                                             &fileRenameInp, rescInfo, 1, new_fn );

            if ( status >= 0 ) {
                rodsLog( LOG_NOTICE,
                         "unbunPhyBunFile: %s has been moved to ORPHAN_DIR.stat=%d",
                         phyBunDir, status );
                status = rsStructFileExtract( rsComm, &structFileOprInp );
            }
            else {
                rodsLog( LOG_ERROR,
                         "unbunPhyBunFile: renameFilePathToNewDir err for %s.stat=%d",
                         phyBunDir, status );
                status = SYS_DIR_IN_VAULT_NOT_EMPTY;
            }
        }
    }
    clearKeyVal( &structFileOprInp.condInput ); // JMC - backport 4632
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unbunPhyBunFile: rsStructFileExtract err for %s. status = %d",
                 objPath, status ); // JMC - backport 4657
    }
    free( structFileOprInp.specColl );

    return ( status );
}

int
remoteUnbunAndRegPhyBunfile( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                             rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteUnbunAndRegPhyBunfile: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcUnbunAndRegPhyBunfile( rodsServerHost->conn, dataObjInp );

    return status;
}

int
rmLinkedFilesInUnixDir( char *phyBunDir ) {
    char subfilePath[MAX_NAME_LEN];
    int status;
    int linkCnt;

    path srcDirPath( phyBunDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        return 0;
    }

    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( subfilePath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            continue;
        }

        if ( is_regular_file( p ) ) {
            if ( ( linkCnt = hard_link_count( p ) ) >= 2 ) {
                /* only unlink files with multi-links */
                unlink( subfilePath );
            }
            else {
                rodsLog( LOG_ERROR,
                         "rmLinkedFilesInUnixDir: st_nlink of %s is only %d",
                         subfilePath, linkCnt );
            }
        }
        else {          /* a directory */
            status = rmLinkedFilesInUnixDir( subfilePath );
            if ( status < 0 ) {
                irods::log( ERROR( status, "rmLinkedFilesInUnixDir failed" ) );
            }
            /* rm subfilePath but not phyBunDir */
            rmdir( subfilePath );
        }
    }
    return 0;
}

int // JMC - backport 4657
rmUnlinkedFilesInUnixDir( char *phyBunDir ) {
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
    int status;
    char subfilePath[MAX_NAME_LEN];
    time_t myTime = time( 0 ) - UNLINK_FILE_AGE; // JMC - backport 4666

    dirPtr = opendir( phyBunDir );
    if ( dirPtr == NULL ) {
        return 0;
    }
    while ( ( myDirent = readdir( dirPtr ) ) != NULL ) {
        if ( strcmp( myDirent->d_name, "." ) == 0 ||
                strcmp( myDirent->d_name, ".." ) == 0 ) {
            continue;
        }
        snprintf( subfilePath, MAX_NAME_LEN, "%s/%s",
                  phyBunDir, myDirent->d_name );
        status = stat( subfilePath, &statbuf );

        if ( status != 0 ) {
            continue;
        }

        if ( ( statbuf.st_mode & S_IFREG ) != 0 ) {
            /* only delete those younger than UNLINK_FILE_AGE. A little
             * safeguard since this routine is very dangerous */
            if ( statbuf.st_nlink == 1 && statbuf.st_mtime > myTime ) {
                unlink( subfilePath );
            }
        }
        else {          /* a directory */
            status = rmUnlinkedFilesInUnixDir( subfilePath );
            /* rm subfilePath but not phyBunDir */
            rmdir( subfilePath );
        }
    }
    closedir( dirPtr );
    return 0;
}

