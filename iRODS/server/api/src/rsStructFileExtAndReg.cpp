/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsStructFileExtAndReg.c. See structFileExtAndReg.h for a description of
 * this API call.*/

#include "reFuncDefs.hpp"
#include "apiHeaderAll.hpp"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "dataObjOpr.hpp"
#include "resource.hpp"
#include "specColl.hpp"
#include "physPath.hpp"
#include "objStat.hpp"
#include "miscServerFunct.hpp"
#include "fileOpr.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "structFileExtAndReg.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"
#include "irods_file_object.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

int
bulkRegUnbunSubfiles( rsComm_t *rsComm, const char*, const std::string& rescHier,
                      char *collection, char *phyBunDir, int flags,
                      genQueryOut_t *attriArray );


int
rsStructFileExtAndReg( rsComm_t *rsComm,
                       structFileExtAndRegInp_t *structFileExtAndRegInp ) {
    int status;
    dataObjInp_t dataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *dataObjInfo;
    int l1descInx;
    char *resc_name;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    char phyBunDir[MAX_NAME_LEN];
    int flags = 0;

    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, structFileExtAndRegInp->objPath, &specCollCache,
                       &structFileExtAndRegInp->condInput );

    resolveLinkedPath( rsComm, structFileExtAndRegInp->collection,
                       &specCollCache, NULL );

    if ( !isSameZone( structFileExtAndRegInp->objPath,
                      structFileExtAndRegInp->collection ) ) {
        return SYS_CROSS_ZONE_MV_NOT_SUPPORTED;
    }

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, structFileExtAndRegInp->objPath,
             MAX_NAME_LEN );

    /* replicate the condInput. may have resource input */
    replKeyVal( &structFileExtAndRegInp->condInput, &dataObjInp.condInput );
    dataObjInp.openFlags = O_RDONLY;

    remoteFlag = getAndConnRemoteZone( rsComm, &dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = rcStructFileExtAndReg( rodsServerHost->conn,
                                        structFileExtAndRegInp );
        return status;
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    if ( getValByKey( &dataObjInp.condInput, RESC_HIER_STR_KW ) == NULL ) {
        irods::error ret = irods::resource_redirect( irods::OPEN_OPERATION, rsComm,
                           &dataObjInp, hier, host, local );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "rsStructFileExtAndReg :: failed in irods::resource_redirect for [";
            msg << dataObjInp.objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    /* open the structured file */
    addKeyVal( &dataObjInp.condInput, NO_OPEN_FLAG_KW, "" );
    l1descInx = _rsDataObjOpen( rsComm, &dataObjInp );

    if ( l1descInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsStructFileExtAndReg: _rsDataObjOpen of %s error. status = %d",
                 dataObjInp.objPath, l1descInx );
        return l1descInx;
    }

    resc_name = L1desc[l1descInx].dataObjInfo->rescName;

    bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx;

    if ( local == REMOTE_HOST ) {
        addKeyVal( &structFileExtAndRegInp->condInput, RESC_NAME_KW, resc_name );

        status = rcStructFileExtAndReg( host->conn, structFileExtAndRegInp );
        rsDataObjClose( rsComm, &dataObjCloseInp );

        return status;
    }

    status = chkCollForExtAndReg( rsComm, structFileExtAndRegInp->collection, NULL );

    if ( status < 0 ) {
        return status;
    }


    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    std::string rescHier = dataObjInfo->rescHier;

    createPhyBundleDir( rsComm, dataObjInfo->filePath, phyBunDir, dataObjInfo->rescHier );

    status = unbunPhyBunFile( rsComm, dataObjInp.objPath, resc_name, // JMC - backport 4657
                              dataObjInfo->filePath, phyBunDir, dataObjInfo->dataType, 0,
                              rescHier.c_str() );

    if ( status == SYS_DIR_IN_VAULT_NOT_EMPTY ) {
        /* rename the phyBunDir */
        char tmp[MAX_NAME_LEN]; // JMC cppcheck - src & dst snprintf
        strcpy( tmp, phyBunDir ); // JMC cppcheck - src & dst snprintf
        snprintf( phyBunDir, MAX_NAME_LEN, "%s.%-d", tmp, ( int ) random() ); // JMC cppcheck - src & dst snprintf
        status = unbunPhyBunFile( rsComm, dataObjInp.objPath, resc_name,
                                  dataObjInfo->filePath, phyBunDir,  dataObjInfo->dataType, 0,
                                  rescHier.c_str() );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsStructFileExtAndReg:unbunPhyBunFile err for %s to dir %s.stat=%d",
                 dataObjInfo->filePath, phyBunDir, status );
        rsDataObjClose( rsComm, &dataObjCloseInp );
        return status;
    }

    if ( getValByKey( &structFileExtAndRegInp->condInput, FORCE_FLAG_KW )
            != NULL ) {
        flags = flags | FORCE_FLAG_FLAG;
    }
    if ( getValByKey( &structFileExtAndRegInp->condInput, BULK_OPR_KW )
            != NULL ) {

        status = bulkRegUnbunSubfiles( rsComm, resc_name, rescHier,
                                       structFileExtAndRegInp->collection, phyBunDir, flags, NULL );
    }
    else {
        status = regUnbunSubfiles( rsComm, resc_name, dataObjInfo->rescHier,
                                   structFileExtAndRegInp->collection, phyBunDir, flags, NULL );
    }

    if ( status == CAT_NO_ROWS_FOUND ) {
        /* some subfiles have been deleted. harmless */
        status = 0;
    }
    else if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsUnbunAndRegPhyBunfile: rsStructFileExtAndReg for dir %s.stat=%d",
                 phyBunDir, status );
    }
    rsDataObjClose( rsComm, &dataObjCloseInp );

    return status;
}

int
chkCollForExtAndReg( rsComm_t *rsComm, char *collection,
                     rodsObjStat_t **rodsObjStatOut ) {
    dataObjInp_t dataObjInp;
    int status;
    rodsObjStat_t *myRodsObjStat = NULL;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, collection, MAX_NAME_LEN );
    status = collStatAllKinds( rsComm, &dataObjInp, &myRodsObjStat );
    if ( status < 0 ) {
        status = rsMkCollR( rsComm, "/", collection );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "chkCollForExtAndReg: rsMkCollR of %s error. status = %d",
                     collection, status );
            return status;
        }
        else {
            status = collStatAllKinds( rsComm, &dataObjInp, &myRodsObjStat );
        }
    }

    if ( status < 0 || NULL == myRodsObjStat ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR,
                 "chkCollForExtAndReg: collStat of %s error. status = %d",
                 dataObjInp.objPath, status );
        return status;
    }
    else if ( myRodsObjStat->specColl != NULL &&
              myRodsObjStat->specColl->collClass != MOUNTED_COLL ) {
        /* only do mounted coll */
        freeRodsObjStat( myRodsObjStat );
        rodsLog( LOG_ERROR,
                 "chkCollForExtAndReg: %s is a struct file collection",
                 dataObjInp.objPath );
        return SYS_STRUCT_FILE_INMOUNTED_COLL;
    }

    if ( myRodsObjStat->specColl == NULL ) {
        status = checkCollAccessPerm( rsComm, collection, ACCESS_DELETE_OBJECT );
    }
    else {
        status = checkCollAccessPerm( rsComm,
                                      myRodsObjStat->specColl->collection, ACCESS_DELETE_OBJECT );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "chkCollForExtAndReg: no permission to write %s, status = %d",
                 collection, status );
        freeRodsObjStat( myRodsObjStat );
    }
    else {
        if ( rodsObjStatOut != NULL ) {
            *rodsObjStatOut = myRodsObjStat;
        }
        else {
            freeRodsObjStat( myRodsObjStat );
        }
    }
    return status;
}

/* regUnbunSubfiles - non bulk version of registering all files in phyBunDir
 * to the collection. Valid values for flags are:
 *      FORCE_FLAG_FLAG.
 */

int
regUnbunSubfiles( rsComm_t *rsComm, const char *_resc_name, const char* rescHier,
                  char *collection, char *phyBunDir, int flags, genQueryOut_t *attriArray ) {
    char subfilePath[MAX_NAME_LEN];
    char subObjPath[MAX_NAME_LEN];
    dataObjInp_t dataObjInp;
    int status;
    int savedStatus = 0;
    rodsLong_t st_size;

    path srcDirPath( phyBunDir );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "regUnbunphySubfiles: opendir error for %s, errno = %d",
                 phyBunDir, errno );
        return UNIX_FILE_OPENDIR_ERR - errno;
    }
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( subfilePath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "regUnbunphySubfiles: stat error for %s, errno = %d",
                     subfilePath, errno );
            savedStatus = UNIX_FILE_STAT_ERR - errno;
            unlink( subfilePath );
            continue;
        }
        // =-=-=-=-=-=-=-
        // JMC - backport 4833
        if ( is_symlink( p ) ) {
            rodsLogError( LOG_ERROR, SYMLINKED_BUNFILE_NOT_ALLOWED,
                          "regUnbunSubfiles: %s is a symlink",
                          subfilePath );
            savedStatus = SYMLINKED_BUNFILE_NOT_ALLOWED;
            continue;
        }
        // =-=-=-=-=-=-=-
        path childPath = p.filename();
        snprintf( subObjPath, MAX_NAME_LEN, "%s/%s",
                  collection, childPath.c_str() );

        if ( is_directory( p ) ) {
            status = rsMkCollR( rsComm, "/", subObjPath );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "regUnbunSubfiles: rsMkCollR of %s error. status = %d",
                         subObjPath, status );
                savedStatus = status;
                continue;
            }
            status = regUnbunSubfiles( rsComm, _resc_name, rescHier,
                                       subObjPath, subfilePath, flags, attriArray );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "regUnbunSubfiles: regUnbunSubfiles of %s error. status=%d",
                         subObjPath, status );
                savedStatus = status;
                continue;
            }
        }
        else if ( is_regular_file( p ) ) {
            st_size = file_size( p );
            status = regSubfile( rsComm, _resc_name, rescHier,
                                 subObjPath, subfilePath, st_size, flags );
            unlink( subfilePath );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "regUnbunSubfiles: regSubfile of %s error. status=%d",
                         subObjPath, status );
                savedStatus = status;
                continue;
            }
        }
    }
    rmdir( phyBunDir );
    return savedStatus;
}

int
regSubfile( rsComm_t *rsComm, const char *_resc_name, const char* rescHier,
            char *subObjPath, char *subfilePath, rodsLong_t dataSize, int flags ) {
    dataObjInfo_t dataObjInfo;
    dataObjInp_t dataObjInp;
    int status;
    int modFlag = 0;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    bzero( &dataObjInfo, sizeof( dataObjInfo ) );
    rstrcpy( dataObjInp.objPath, subObjPath, MAX_NAME_LEN );
    rstrcpy( dataObjInfo.objPath, subObjPath, MAX_NAME_LEN );
    rstrcpy( dataObjInfo.rescName, _resc_name, NAME_LEN );
    rstrcpy( dataObjInfo.rescHier, rescHier, MAX_NAME_LEN );
    rstrcpy( dataObjInfo.dataType, "generic", NAME_LEN );

    dataObjInfo.dataSize = dataSize;
    dataObjInfo.replStatus = 1;

    status = getFilePathName( rsComm, &dataObjInfo, &dataObjInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regSubFile: getFilePathName err for %s. status = %d",
                 dataObjInp.objPath, status );
        return status;
    }

    path p( dataObjInfo.filePath );
    if ( exists( p ) ) {
        if ( is_directory( p ) ) {
            return SYS_PATH_IS_NOT_A_FILE;
        }

        if ( chkOrphanFile( rsComm, dataObjInfo.filePath, _resc_name,
                            &dataObjInfo ) > 0 ) {
            /* an orphan file. just rename it */
            fileRenameInp_t fileRenameInp;
            bzero( &fileRenameInp, sizeof( fileRenameInp ) );
            rstrcpy( fileRenameInp.oldFileName, dataObjInfo.filePath,
                     MAX_NAME_LEN );
            char new_fn[ MAX_NAME_LEN ];
            status = renameFilePathToNewDir( rsComm, ORPHAN_DIR,
                                             &fileRenameInp, 1, new_fn );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "regSubFile: renameFilePathToNewDir err for %s. status = %d",
                         fileRenameInp.oldFileName, status );
                return status;
            }
        }
        else {
            /* not an orphan file */
            if ( ( flags & FORCE_FLAG_FLAG ) != 0 && dataObjInfo.dataId > 0 &&
                    strcmp( dataObjInfo.objPath, subObjPath ) == 0 ) {
                /* overwrite the current file */
                modFlag = 1;
                unlink( dataObjInfo.filePath );
            }
            else {
                status = SYS_COPY_ALREADY_IN_RESC;
                rodsLog( LOG_ERROR,
                         "regSubFile: phypath %s is already in use. status = %d",
                         dataObjInfo.filePath, status );
                return status;
            }
        }
    }
    /* make the necessary dir */
    status = mkDirForFilePath(
        rsComm,
        0,
        dataObjInfo.filePath,
        dataObjInfo.rescHier,
        getDefDirMode() );
    if ( status ) {
        rodsLog( LOG_ERROR, "mkDirForFilePath failed in regSubfile with status %d.", status );
        return status;
    }
    /* add a link */

#ifndef windows_platform   /* Windows does not support link */
    status = link( subfilePath, dataObjInfo.filePath );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regSubFile: link error %s to %s. errno = %d",
                 subfilePath, dataObjInfo.filePath, errno );
        return UNIX_FILE_LINK_ERR - errno;
    }
#endif

    if ( modFlag == 0 ) {
        status = svrRegDataObj( rsComm, &dataObjInfo );
        // =-=-=-=-=-=-=-
        // need to call modified under the covers for unbundle
        // otherwise the resource hier is not properly notified
        irods::file_object_ptr file_obj(
            new irods::file_object(
                rsComm,
                &dataObjInfo ) );

        irods::error ret = fileModified( rsComm, file_obj );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << " Failed to signal resource that the data object \"";
            msg << dataObjInfo.objPath;
            msg << " was modified.";
            ret = PASSMSG( msg.str(), ret );
            irods::log( ret );
            status = ret.code();
        }

    }
    else {
        char tmpStr[MAX_NAME_LEN];
        modDataObjMeta_t modDataObjMetaInp;
        keyValPair_t regParam;

        bzero( &modDataObjMetaInp, sizeof( modDataObjMetaInp ) );
        bzero( &regParam, sizeof( regParam ) );
        snprintf( tmpStr, MAX_NAME_LEN, "%lld", dataSize );
        addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
        addKeyVal( &regParam, ALL_REPL_STATUS_KW, tmpStr );
        snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
        addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );

        modDataObjMetaInp.dataObjInfo = &dataObjInfo;
        modDataObjMetaInp.regParam = &regParam;

        // =-=-=-=-=-=-=-
        // this path does call fileModified
        status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );

        clearKeyVal( &regParam );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regSubFile: svrRegDataObj of %s. errno = %d",
                 dataObjInfo.objPath, errno );
        unlink( dataObjInfo.filePath );
    }
    else {
        ruleExecInfo_t rei;
        dataObjInp_t dataObjInp;
        bzero( &dataObjInp, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, dataObjInfo.objPath, MAX_NAME_LEN );
        initReiWithDataObjInp( &rei, rsComm, &dataObjInp );
        rei.doi = &dataObjInfo;
        rei.status = applyRule( "acPostProcForTarFileReg", NULL, &rei,
                                NO_SAVE_REI );
        if ( rei.status < 0 ) {
            rodsLogError( LOG_ERROR, rei.status,
                          "regSubFile: acPostProcForTarFileReg error for %s. status = %d",
                          dataObjInfo.objPath );
        }
    }
    return status;
}

