/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsStructFileBundle.c. See structFileBundle.h for a description of
 * this API call.*/

#include "reFuncDefs.hpp"
#include "apiHeaderAll.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "miscServerFunct.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_redirect.hpp"

int
rsStructFileBundle( rsComm_t *rsComm,
                    structFileExtAndRegInp_t *structFileBundleInp ) {
    int status;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    dataObjInp_t dataObjInp;


    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, structFileBundleInp->objPath,
             MAX_NAME_LEN );

    remoteFlag = getAndConnRemoteZone( rsComm, &dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = rcStructFileBundle( rodsServerHost->conn,
                                     structFileBundleInp );
        return status;
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    dataObjInp_t      data_inp;
    bzero( &data_inp, sizeof( data_inp ) );
    rstrcpy( data_inp.objPath, structFileBundleInp->objPath, MAX_NAME_LEN );
    copyKeyValPairStruct( &structFileBundleInp->condInput, &data_inp.condInput );
    if ( getValByKey( &structFileBundleInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        irods::error ret = irods::resource_redirect( irods::CREATE_OPERATION, rsComm,
                           &data_inp, hier, host, local );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "rsStructFileBundle :: failed in irods::resource_redirect for [";
            msg << &data_inp.objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &structFileBundleInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    if ( LOCAL_HOST == local ) {
        status = _rsStructFileBundle( rsComm, structFileBundleInp );
    }
    else {
        status = rcStructFileBundle( host->conn, structFileBundleInp );
    } // else remote host


    return status;
}

int _rsStructFileBundle( rsComm_t*                 rsComm,
                         structFileExtAndRegInp_t* structFileBundleInp ) {
    int status;
    int handleInx;
    char phyBunDir[MAX_NAME_LEN];
    char tmpPath[MAX_NAME_LEN];
    int l1descInx;
    char* dataType = 0; // JMC - backport 4664
    openedDataObjInp_t dataObjCloseInp;

    // =-=-=-=-=-=-=-
    // create an empty data obj
    dataObjInp_t dataObjInp;
    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    dataObjInp.openFlags = O_WRONLY;

    // =-=-=-=-=-=-=-
    // get the data type of the structured file
    dataType = getValByKey( &structFileBundleInp->condInput, DATA_TYPE_KW );

    // =-=-=-=-=-=-=-
    // ensure that the file name will end in .zip, if necessary
    if ( dataType != NULL && strstr( dataType, ZIP_DT_STR ) != NULL ) {
        int len = strlen( structFileBundleInp->objPath );
        if ( strcmp( &structFileBundleInp->objPath[len - 4], ".zip" ) != 0 ) {
            strcat( structFileBundleInp->objPath, ".zip" );
        }
    }

    // =-=-=-=-=-=-=-
    // capture the object path in the data obj struct
    rstrcpy( dataObjInp.objPath, structFileBundleInp->objPath, MAX_NAME_LEN );

    // =-=-=-=-=-=-=-
    // replicate the condInput. may have resource input
    replKeyVal( &structFileBundleInp->condInput, &dataObjInp.condInput );

    // =-=-=-=-=-=-=-
    // open the file if we are in an add operation, otherwise create the new file
    if ( ( structFileBundleInp->oprType & ADD_TO_TAR_OPR ) != 0 ) { // JMC - backport 4643
        l1descInx = rsDataObjOpen( rsComm, &dataObjInp );

    }
    else {
        l1descInx = rsDataObjCreate( rsComm, &dataObjInp );

    }

    // =-=-=-=-=-=-=-
    // error check create / open
    if ( l1descInx < 0 ) {
        rodsLog( LOG_ERROR, "rsStructFileBundle: rsDataObjCreate of %s error. status = %d",
                 dataObjInp.objPath, l1descInx );
        return l1descInx;
    }

    // =-=-=-=-=-=-=-
    // FIXME :: Why, when we replicate them above?
    clearKeyVal( &dataObjInp.condInput ); // JMC - backport 4637
    // ???? l3Close (rsComm, l1descInx);

    // =-=-=-=-=-=-=-
    // zip does not like a zero length file as target
    //L1desc[ l1descInx ].l3descInx = 0;
    //if( dataType != NULL && strstr( dataType, ZIP_DT_STR ) != NULL ) {
    //    if( ( structFileBundleInp->oprType & ADD_TO_TAR_OPR) == 0 ) { // JMC - backport 4643
    //        l3Unlink( rsComm, L1desc[l1descInx].dataObjInfo );
    //    }
    //}

    // =-=-=-=-=-=-=-
    // check object permissions / stat
    chkObjPermAndStat_t chkObjPermAndStatInp;
    memset( &chkObjPermAndStatInp, 0, sizeof( chkObjPermAndStatInp ) );
    rstrcpy( chkObjPermAndStatInp.objPath, structFileBundleInp->collection, MAX_NAME_LEN );
    chkObjPermAndStatInp.flags = CHK_COLL_FOR_BUNDLE_OPR;
    addKeyVal( &chkObjPermAndStatInp.condInput, RESC_NAME_KW,     L1desc[l1descInx].dataObjInfo->rescName );

    // =-=-=-=-=-=-=-
    // get the resc hier string
    std::string resc_hier;
    char* resc_hier_ptr = getValByKey( &structFileBundleInp->condInput, RESC_HIER_STR_KW );
    if ( !resc_hier_ptr ) {
        rodsLog( LOG_NOTICE, "_rsStructFileBundle :: RESC_HIER_STR_KW is NULL" );
    }
    else {
        addKeyVal( &chkObjPermAndStatInp.condInput, RESC_HIER_STR_KW, resc_hier_ptr );
        resc_hier = resc_hier_ptr;
    }

    status = rsChkObjPermAndStat( rsComm, &chkObjPermAndStatInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rsStructFileBundle: rsChkObjPermAndStat of %s error. stat = %d",
                 chkObjPermAndStatInp.objPath, status );
        dataObjCloseInp.l1descInx = l1descInx;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        return status;
    }

    clearKeyVal( &chkObjPermAndStatInp.condInput );

    // =-=-=-=-=-=-=-
    // create the special hidden directory where the bundling happens
    createPhyBundleDir( rsComm, L1desc[ l1descInx ].dataObjInfo->filePath, phyBunDir, L1desc[ l1descInx ].dataObjInfo->rescHier );

    // =-=-=-=-=-=-=-
    // build a collection open input structure
    collInp_t collInp;
    bzero( &collInp, sizeof( collInp ) );
    collInp.flags = RECUR_QUERY_FG | VERY_LONG_METADATA_FG | NO_TRIM_REPL_FG | INCLUDE_CONDINPUT_IN_QUERY;
    rstrcpy( collInp.collName, structFileBundleInp->collection, MAX_NAME_LEN );
    addKeyVal( &collInp.condInput, RESC_NAME_KW, L1desc[ l1descInx ].dataObjInfo->rescName );

    rodsLog( LOG_DEBUG, "rsStructFileBundle: calling rsOpenCollection for [%s]", structFileBundleInp->collection );

    // =-=-=-=-=-=-=-
    // open the collection from which we will bundle
    handleInx = rsOpenCollection( rsComm, &collInp );
    if ( handleInx < 0 ) {
        rodsLog( LOG_ERROR, "rsStructFileBundle: rsOpenCollection of %s error. status = %d",
                 collInp.collName, handleInx );
        rmdir( phyBunDir );
        return handleInx;
    }

    // =-=-=-=-=-=-=-
    // preserve the collection path?
    int collLen = 0;
    if ( ( structFileBundleInp->oprType & PRESERVE_COLL_PATH ) != 0 ) {
        // =-=-=-=-=-=-=-
        // preserve the last entry of the coll path
        char* tmpPtr = collInp.collName;
        int   tmpLen = 0;
        collLen = 0;

        // =-=-=-=-=-=-=-
        // find length to the last '/'
        while ( *tmpPtr != '\0' ) {
            if ( *tmpPtr == '/' ) {
                collLen = tmpLen;
            }

            tmpLen++;
            tmpPtr++;
        }

    }
    else {
        collLen = strlen( collInp.collName );

    }

    // =-=-=-=-=-=-=-
    // preserve the collection path?
    collEnt_t* collEnt = NULL;
    while ( ( status = rsReadCollection( rsComm, &handleInx, &collEnt ) ) >= 0 ) {
        if ( NULL == collEnt ) { // JMC cppcheck - nullptr
            rodsLog( LOG_ERROR, "rsStructFileBundle: collEnt is NULL" );
            continue;
        }

        // =-=-=-=-=-=-=-
        // entry is a data object
        if ( collEnt->objType == DATA_OBJ_T ) {
            if ( collEnt->collName[collLen] == '\0' ) {
                snprintf(
                    tmpPath,
                    MAX_NAME_LEN,
                    "%s/%s",
                    phyBunDir,
                    collEnt->dataName );
            }
            else {
                snprintf(
                    tmpPath,
                    MAX_NAME_LEN, "%s/%s/%s",
                    phyBunDir,
                    collEnt->collName + collLen + 1,
                    collEnt->dataName );
                status = mkDirForFilePath(
                    rsComm,
                    strlen( phyBunDir ),
                    tmpPath,
                    collEnt->resc_hier,
                    getDefDirMode() );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "mkDirForFilePath failed in _rsStructFileBundle with status %d", status );
                    return status;
                }
            }

            // =-=-=-=-=-=-=-
            // filter out any possible replicas that are not on this resource
            if ( resc_hier == collEnt->resc_hier ) {
                // =-=-=-=-=-=-=-
                // add a link
                status = link( collEnt->phyPath, tmpPath );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "rsStructFileBundle: link error %s to %s. errno = %d",
                             collEnt->phyPath, tmpPath, errno );
                    rmLinkedFilesInUnixDir( phyBunDir );
                    rmdir( phyBunDir );
                    free( collEnt );
                    return UNIX_FILE_LINK_ERR - errno;
                }
                else {
                    //rodsLog( LOG_NOTICE, "_rsStructFileBundle - LINK  [%s] on resc [%s]", collEnt->phyPath, collEnt->resc_hier );
                }
            }
            else {
                //rodsLog( LOG_NOTICE, "_rsStructFileBundle - skipping [%s] on resc [%s]", collEnt->phyPath, collEnt->resc_hier );
            }
        }
        else {
            // =-=-=-=-=-=-=-
            // entry is a collection
            if ( ( int ) strlen( collEnt->collName ) + 1 <= collLen ) {
                free( collEnt );
                continue;
            }
            snprintf( tmpPath, MAX_NAME_LEN, "%s/%s", phyBunDir, collEnt->collName + collLen );
            status = mkFileDirR( rsComm, strlen( phyBunDir ), tmpPath, resc_hier.c_str(), getDefDirMode() );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR, "mkFileDirR failed in _rsStructFileBundle with status %d", status );
                free( collEnt );
                return status;
            }
        } // else

        if ( collEnt != NULL ) {
            free( collEnt );
            collEnt = NULL;
        }

    } // while

    // =-=-=-=-=-=-=-
    // clean up key vals and close the collection
    clearKeyVal( &collInp.condInput );
    rsCloseCollection( rsComm, &handleInx );

    // =-=-=-=-=-=-=-
    // call the helper function to do the actual bundling
    status = phyBundle( rsComm, L1desc[l1descInx].dataObjInfo, phyBunDir,
                        collInp.collName, structFileBundleInp->oprType ); // JMC - backport 4643

    int savedStatus = 0;
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "rsStructFileBundle: phyBundle of %s error. stat = %d",
                 L1desc[ l1descInx ].dataObjInfo->objPath, status );
        L1desc[ l1descInx ].bytesWritten = 0;
        savedStatus = status;
    }
    else {
        // mark it was written so the size would be adjusted
        L1desc[ l1descInx ].bytesWritten = 1;
    }

    // =-=-=-=-=-=-=-
    // clean up after the bundle directory
    rmLinkedFilesInUnixDir( phyBunDir );
    rmdir( phyBunDir );

    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose( rsComm, &dataObjCloseInp );
    if ( status >= 0 ) {
        return savedStatus;
    }

    return status;
}

int
remoteStructFileBundle( rsComm_t *rsComm,
                        structFileExtAndRegInp_t *structFileBundleInp, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteStructFileBundle: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcStructFileBundle( rodsServerHost->conn, structFileBundleInp );
    return status;
}

