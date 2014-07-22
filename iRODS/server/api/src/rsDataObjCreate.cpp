/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCreate.h for a description of this API call.*/

#include "dataObjCreate.hpp"
#include "dataObjCreateAndStat.hpp"
#include "dataObjOpen.hpp"
#include "fileCreate.hpp"
#include "subStructFileCreate.hpp"
#include "rodsLog.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "specColl.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "dataObjUnlink.hpp"
#include "dataObjLock.hpp" // JMC - backport 4604
#include "regDataObj.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "getRemoteZoneResc.hpp"
#include "getRescQuota.hpp"
#include "icatHighLevelRoutines.hpp"
#include "reFuncDefs.hpp"

#include "irods_hierarchy_parser.hpp"


// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"

/* rsDataObjCreate - handle dataObj create request.
 *
 * The NO_OPEN_FLAG_KW in condInput specifies that no physical create
 * and registration will be done.
 *
 * return value -  > 2 - success with phy open
 *                < 0 - failure
 */

int
rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int l1descInx;
    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    char *lockType = NULL; // JMC - backport 4604
    int lockFd = -1; // JMC - backport 4604

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE );
    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        openStat_t *openStat = NULL;
        addKeyVal( &dataObjInp->condInput, CROSS_ZONE_CREATE_KW, "" );
        status = rcDataObjCreateAndStat( rodsServerHost->conn, dataObjInp, &openStat );

        /* rm it to avoid confusion */
        rmKeyVal( &dataObjInp->condInput, CROSS_ZONE_CREATE_KW );
        if ( status < 0 ) {
            return status;
        }
        l1descInx = allocAndSetL1descForZoneOpr( status, dataObjInp, rodsServerHost, openStat );

        if ( openStat != NULL ) {
            free( openStat );
        }
        return ( l1descInx );
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( NULL == resc_hier ) {
        std::string       hier;
        irods::error ret = irods::resolve_resource_hierarchy( irods::CREATE_OPERATION, rsComm,
                           dataObjInp, hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    lockType = getValByKey( &dataObjInp->condInput, LOCK_TYPE_KW );
    if ( lockType != NULL ) {
        lockFd = rsDataObjLock( rsComm, dataObjInp );
        if ( lockFd >= 0 ) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW );
        }
        else {
            rodsLogError( LOG_ERROR, lockFd,
                          "rsDataObjCreate: rsDataObjLock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType );
            return lockFd;
        }
    }

    // =-=-=-=-=-=-=-
    // Gets here means local zone operation stat dataObj
    addKeyVal( &dataObjInp->condInput, SEL_OBJ_TYPE_KW, "dataObj" );

    status = rsObjStat( rsComm, dataObjInp, &rodsObjStatOut );

    if ( rodsObjStatOut != NULL && rodsObjStatOut->objType == COLL_OBJ_T ) {
        if ( lockFd >= 0 ) {
            rsDataObjUnlock( rsComm, dataObjInp, lockFd );    // JMC - backport 4604
        }
        return ( USER_INPUT_PATH_ERR );
    }

    if ( rodsObjStatOut                      != NULL &&
            rodsObjStatOut->specColl            != NULL &&
            rodsObjStatOut->specColl->collClass == LINKED_COLL ) {
        /*  should not be here because if has been translated */
        if ( lockFd >= 0 ) {
            rsDataObjUnlock( rsComm, dataObjInp, lockFd ); // JMC - backport 4604
        }

        return SYS_COLL_LINK_PATH_ERR;
    }


    if ( rodsObjStatOut  == NULL                     ||
            ( rodsObjStatOut->objType  == UNKNOWN_OBJ_T &&
              rodsObjStatOut->specColl == NULL ) ) {
        /* does not exist. have to create one */
        /* use L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY instead */
        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */
        l1descInx = _rsDataObjCreate( rsComm, dataObjInp );

    }
    else if ( rodsObjStatOut->specColl != NULL &&
              rodsObjStatOut->objType == UNKNOWN_OBJ_T ) {

        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */
        l1descInx = specCollSubCreate( rsComm, dataObjInp );
    }
    else {

        /* dataObj exist */
        if ( getValByKey( &dataObjInp->condInput, FORCE_FLAG_KW ) != NULL ) {
            dataObjInp->openFlags |= O_TRUNC | O_RDWR;

            // =-=-=-=-=-=-=-
            // re-determine the resource hierarchy since this is an open instead of a create
            std::string       hier;
            irods::error ret = irods::resolve_resource_hierarchy( irods::WRITE_OPERATION,
                               rsComm, dataObjInp, hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " :: failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjInp->objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );
            std::string top_resc;
            irods::hierarchy_parser parser;
            parser.set_string( hier );
            parser.first_resc( top_resc );
            addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW, top_resc.c_str() );
            l1descInx = _rsDataObjOpen( rsComm, dataObjInp );

        }
        else {
            l1descInx = OVERWRITE_WITHOUT_FORCE_FLAG;
        }
    }

    if ( rodsObjStatOut != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    if ( lockFd >= 0 ) {
        if ( l1descInx >= 0 ) {
            L1desc[l1descInx].lockFd = lockFd;
        }
        else {
            rsDataObjUnlock( rsComm, dataObjInp, lockFd );
        }
    }

    // =-=-=-=-=-=-=-
    return ( l1descInx );
}

int
_rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {

    int status;
    rescGrpInfo_t* myRescGrpInfo  = 0;
    int l1descInx;

    /* query rcat for resource info and sort it */
    status = getRescGrpForCreate( rsComm, dataObjInp, &myRescGrpInfo );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "_rsDataObjCreate : failed in call to getRescGrpForCreate. status = %d", status );
        return status;
    }

    status = l1descInx = _rsDataObjCreateWithRescInfo( rsComm, dataObjInp, myRescGrpInfo->rescInfo, myRescGrpInfo->rescGroupName );

    delete myRescGrpInfo->rescInfo;
    delete myRescGrpInfo;

    // JMC - legacy resource - if (status < 0) {
    if ( status >= 0 ) {
        return ( status );
    }
    else {
        rodsLog( LOG_NOTICE,
                 "rsDataObjCreate: Internal error" );
        return ( SYS_INTERNAL_NULL_INPUT_ERR );
    }
}

int
specCollSubCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    int l1descInx;
    dataObjInfo_t *dataObjInfo = NULL;

    status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                    WRITE_COLL_PERM, 0, &dataObjInfo );
    if ( dataObjInfo == NULL ) { // JMC cppcheck
        rodsLog( LOG_ERROR, "specCollSubCreate :: dataObjInp is null" );
        return status;
    }
    if ( status >= 0 ) {
        rodsLog( LOG_ERROR,
                 "specCollSubCreate: phyPath %s already exist",
                 dataObjInfo->filePath );
        freeDataObjInfo( dataObjInfo );
        return ( SYS_COPY_ALREADY_IN_RESC );
    }
    else if ( status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
        return ( status );
    }

    l1descInx = allocL1desc();

    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    dataObjInfo->replStatus = NEWLY_CREATED_COPY;
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize );

    if ( getValByKey( &dataObjInp->condInput, NO_OPEN_FLAG_KW ) == NULL ) {
        status = dataCreate( rsComm, l1descInx );
        if ( status < 0 ) {
            freeL1desc( l1descInx );
            return ( status );
        }
    }

    return l1descInx;
}

/* _rsDataObjCreateWithRescInfo - Create a single copy of the data Object
 * given the rescInfo.
 *
 * return l1descInx.
 */

int
_rsDataObjCreateWithRescInfo(
    rsComm_t*     rsComm,
    dataObjInp_t* dataObjInp,
    rescInfo_t*   rescInfo,
    char*         rescGroupName ) {

    dataObjInfo_t *dataObjInfo;
    int l1descInx;
    int status;

    l1descInx = allocL1desc();
    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    dataObjInfo = ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    initDataObjInfoWithInp( dataObjInfo, dataObjInp );

    // =-=-=-=-=-=-=-
    // honor the purge flag
    if ( getValByKey( &dataObjInp->condInput, PURGE_CACHE_KW ) != NULL ) { // JMC - backport 4537
        L1desc[l1descInx].purgeCacheFlag = 1;
    }

    dataObjInfo->rescInfo = new rescInfo_t;
    memcpy( dataObjInfo->rescInfo, rescInfo, sizeof( rescInfo_t ) );
    rstrcpy( dataObjInfo->rescName, rescInfo->rescName, NAME_LEN );
    rstrcpy( dataObjInfo->rescGroupName, rescGroupName, NAME_LEN );

    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( resc_hier ) {
        rstrcpy( dataObjInfo->rescHier, resc_hier, MAX_NAME_LEN );

    }
    else {
        rstrcpy( dataObjInfo->rescHier, rescInfo->rescName, MAX_NAME_LEN ); // in kw else

    }

    dataObjInfo->replStatus = NEWLY_CREATED_COPY; // JMC - backport 4754
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize );

    status = getFilePathName( rsComm, dataObjInfo, L1desc[l1descInx].dataObjInp );

    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return ( status );
    }

    /* output of _rsDataObjCreate - filePath stored in
     * dataObjInfo struct */

    if ( getValByKey( &dataObjInp->condInput, NO_OPEN_FLAG_KW ) != NULL ) {

        /* don't actually physically open the file */
        status = 0;
    }
    else {

        status = dataObjCreateAndReg( rsComm, l1descInx );
    }

    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return ( status );
    }
    else {
        return ( l1descInx );
    }
}

/* dataObjCreateAndReg - Given the l1descInx, physically the file (dataCreate)
 * and register the new data object with the rcat
 */

int
dataObjCreateAndReg( rsComm_t *rsComm, int l1descInx ) {

    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;

    status = dataCreate( rsComm, l1descInx );

    if ( status < 0 ) {
        return ( status );
    }

    /* only register new copy */
    status = svrRegDataObj( rsComm, myDataObjInfo );
    if ( status < 0 ) {
        l3Unlink( rsComm, myDataObjInfo );
        rodsLog( LOG_NOTICE,
                 "dataObjCreateAndReg: rsRegDataObj for %s failed, status = %d",
                 myDataObjInfo->objPath, status );
        return ( status );
    }
    else {
        myDataObjInfo->replNum = status;
        return ( 0 );
    }
}

/* dataCreate - given the l1descInx, physically create the file
 * and save the l3descInx in L1desc[l1descInx].
 */

int
dataCreate( rsComm_t *rsComm, int l1descInx ) {
    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;

    /* should call resolveHostForTansfer to find host. gateway rodsServerHost
     * should be in l3desc */
    status = l3Create( rsComm, l1descInx );

    if ( status <= 0 ) {
        rodsLog( LOG_NOTICE,
                 "dataCreate: l3Create of %s failed, status = %d",
                 myDataObjInfo->filePath, status );
        return ( status );
    }
    else {
        L1desc[l1descInx].l3descInx = status;
    }

    return ( 0 );
}

int
l3Create( rsComm_t *rsComm, int l1descInx ) {
    dataObjInfo_t *dataObjInfo;
    int l3descInx;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Create - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }


    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        l3descInx = rsSubStructFileCreate( rsComm, &subFile );
    }
    else {
        /* normal or mounted file */
        l3descInx = l3CreateByObjInfo( rsComm, L1desc[l1descInx].dataObjInp,
                                       L1desc[l1descInx].dataObjInfo );
    }

    return ( l3descInx );
}

int
l3CreateByObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   dataObjInfo_t *dataObjInfo ) {

    int chkType = 0; // JMC - backport 4774

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3CreateByObjInfo - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }


    fileCreateInp_t fileCreateInp;
    memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
    rstrcpy( fileCreateInp.resc_name_, location.c_str(),      MAX_NAME_LEN );
    rstrcpy( fileCreateInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileCreateInp.objPath,    dataObjInfo->objPath,  MAX_NAME_LEN );
    rstrcpy( fileCreateInp.addr.hostAddr, location.c_str(), NAME_LEN );

    rstrcpy( fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    fileCreateInp.mode = getFileMode( dataObjInp );
    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    chkType = getchkPathPerm( rsComm, dataObjInp, dataObjInfo );
    copyFilesystemMetadata( &dataObjInfo->condInput,
                            &fileCreateInp.condInput );
    if ( chkType == DISALLOW_PATH_REG ) {
        clearKeyVal( &fileCreateInp.condInput );
        return PATH_REG_NOT_ALLOWED;
    }
    else if ( chkType == NO_CHK_PATH_PERM ) {
        fileCreateInp.otherFlags |= NO_CHK_PERM_FLAG;  // JMC - backport 4758
    }
    rstrcpy( fileCreateInp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );

    //loop until we find a valid filename
    int retryCnt = 0;
    int l3descInx;
    do {
        fileCreateOut_t* create_out = NULL;
        l3descInx = rsFileCreate( rsComm, &fileCreateInp, &create_out );

        // update the dataObjInfo with the potential changes made by the resource - hcj
        if ( create_out != NULL ) {
            rstrcpy( dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN );
            rstrcpy( dataObjInfo->filePath, create_out->file_name, MAX_NAME_LEN );
            free( create_out );
        }

        //update the filename in case of a retry
        rstrcpy( fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
        retryCnt++;
     } while ( getErrno( l3descInx ) == EEXIST &&
            resolveDupFilePath( rsComm, dataObjInfo, dataObjInp ) >= 0 &&
            l3descInx <= 2 && retryCnt < 100 );
    clearKeyVal( &fileCreateInp.condInput );
    return ( l3descInx );
}

/* getRescGrpForCreate - given the resource input in dataObjInp, get the
 * rescGrpInfo_t of the resource after applying the acSetRescSchemeForCreate
 * rule.
 * Return 1 of the "random" sorting scheme is used. Otherwise return 0
 * or an error code.
 */

int getRescGrpForCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rescGrpInfo_t** myRescGrpInfo ) {
    int            status;
    ruleExecInfo_t rei;

    /* query rcat for resource info and sort it */
    initReiWithDataObjInp( &rei, rsComm, dataObjInp );

    if ( dataObjInp->oprType == REPLICATE_OPR ) { // JMC - backport 4660
        status = applyRule( "acSetRescSchemeForRepl", NULL, &rei, NO_SAVE_REI );

    }
    else {
        status = applyRule( "acSetRescSchemeForCreate", NULL, &rei, NO_SAVE_REI );

    }

    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }

        rodsLog( LOG_NOTICE, "getRescGrpForCreate:acSetRescSchemeForCreate error for %s,status=%d",
                 dataObjInp->objPath, status );

        return ( status );
    }

    if ( rei.rgi == NULL ) {
        /* def resc group has not been initialized yet */
        // JMC - legacy resource status = setDefaultResc (rsComm, NULL, NULL, &dataObjInp->condInput, myRescGrpInfo );
        //if( !(*myRescGrpInfo) ) {
        ( *myRescGrpInfo ) = new rescGrpInfo_t;
        bzero( ( *myRescGrpInfo ), sizeof( rescGrpInfo_t ) );
        ( *myRescGrpInfo )->rescInfo = new rescInfo_t;
        //}

        irods::error set_err = irods::set_default_resource( rsComm, "", "", &dataObjInp->condInput, *( *myRescGrpInfo ) );
        if ( !set_err.ok() ) {
            delete ( *myRescGrpInfo )->rescInfo;
            delete ( *myRescGrpInfo );
            irods::log( PASS( set_err ) );
            return SYS_INVALID_RESC_INPUT;
        }

    }
    else {
        *myRescGrpInfo = rei.rgi;
    }

    status = setRescQuota( rsComm, dataObjInp->objPath, myRescGrpInfo, dataObjInp->dataSize );

    if ( status == SYS_RESC_QUOTA_EXCEEDED ) {
        if ( rei.rgi == NULL ) {
            delete ( *myRescGrpInfo )->rescInfo;
            delete ( *myRescGrpInfo );
        }
        return SYS_RESC_QUOTA_EXCEEDED;
    }

    return 0; // JMC - should this be 1 per above block?
}

