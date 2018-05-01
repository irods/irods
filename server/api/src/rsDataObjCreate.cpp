/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjCreate.h for a description of this API call.*/

#include "dataObjCreate.h"
#include "dataObjCreateAndStat.h"
#include "dataObjOpen.h"
#include "fileCreate.h"
#include "subStructFileCreate.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "specColl.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "dataObjUnlink.h"
#include "dataObjLock.h"
#include "regDataObj.h"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "icatHighLevelRoutines.hpp"
#include "rsDataObjCreate.hpp"
#include "apiNumber.h"
#include "rsObjStat.hpp"
#include "rsDataObjOpen.hpp"
#include "rsRegDataObj.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsSubStructFileCreate.hpp"
#include "rsFileCreate.hpp"
#include "rsGetRescQuota.hpp"

#include "irods_hierarchy_parser.hpp"


// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_server_api_call.hpp"

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
    rodsObjStat_t *rodsObjStatOut = nullptr;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = nullptr;
    char *lockType = nullptr; // JMC - backport 4604
    int lockFd = -1; // JMC - backport 4604

    irods::error ret = validate_logical_path( dataObjInp->objPath );
    if( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE );
    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        openStat_t *openStat = nullptr;
        addKeyVal( &dataObjInp->condInput, CROSS_ZONE_CREATE_KW, "" );
        status = rcDataObjCreateAndStat( rodsServerHost->conn, dataObjInp, &openStat );

        /* rm it to avoid confusion */
        rmKeyVal( &dataObjInp->condInput, CROSS_ZONE_CREATE_KW );
        if ( status < 0 ) {
            return status;
        }
        l1descInx = allocAndSetL1descForZoneOpr( status, dataObjInp, rodsServerHost, openStat );

        if ( openStat != nullptr ) {
            free( openStat );
        }
        return l1descInx;
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( nullptr == resc_hier ) {
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
    if ( lockType != nullptr ) {
        lockFd = irods::server_api_call(
                     DATA_OBJ_LOCK_AN,
                     rsComm,
                     dataObjInp,
                     nullptr,
                     ( void** ) nullptr,
                     nullptr );
        if ( lockFd >= 0 ) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW );
        }
        else {
            rodsLogError( LOG_ERROR, lockFd,
                          "rsDataObjCreate: lock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType );
            return lockFd;
        }
    }

    // =-=-=-=-=-=-=-
    // Gets here means local zone operation stat dataObj
    addKeyVal( &dataObjInp->condInput, SEL_OBJ_TYPE_KW, "dataObj" );

    status = rsObjStat( rsComm, dataObjInp, &rodsObjStatOut );

    if ( rodsObjStatOut != nullptr && rodsObjStatOut->objType == COLL_OBJ_T ) {
        if ( lockFd >= 0 ) {
            char fd_string[NAME_LEN];
            snprintf( fd_string, sizeof( fd_string ), "%-d", lockFd );
            addKeyVal( &dataObjInp->condInput, LOCK_FD_KW, fd_string );
            irods::server_api_call(
                DATA_OBJ_UNLOCK_AN,
                rsComm,
                dataObjInp,
                nullptr,
                ( void** ) nullptr,
                nullptr );
        }
        freeRodsObjStat( rodsObjStatOut );
        return USER_INPUT_PATH_ERR;
    }

    if ( rodsObjStatOut                      != nullptr &&
            rodsObjStatOut->specColl            != nullptr &&
            rodsObjStatOut->specColl->collClass == LINKED_COLL ) {
        /*  should not be here because if has been translated */
        if ( lockFd >= 0 ) {
            char fd_string[NAME_LEN];
            snprintf( fd_string, sizeof( fd_string ), "%-d", lockFd );
            addKeyVal( &dataObjInp->condInput, LOCK_FD_KW, fd_string );
            irods::server_api_call(
                DATA_OBJ_UNLOCK_AN,
                rsComm,
                dataObjInp,
                nullptr,
                ( void** ) nullptr,
                nullptr );
        }

        freeRodsObjStat( rodsObjStatOut );
        return SYS_COLL_LINK_PATH_ERR;
    }


    if ( rodsObjStatOut  == nullptr                     ||
            ( rodsObjStatOut->objType  == UNKNOWN_OBJ_T &&
              rodsObjStatOut->specColl == nullptr ) ) {
        /* does not exist. have to create one */
        /* use L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY instead */
        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */
        l1descInx = _rsDataObjCreate( rsComm, dataObjInp );

    }
    else if ( rodsObjStatOut->specColl != nullptr &&
              rodsObjStatOut->objType == UNKNOWN_OBJ_T ) {

        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */
        l1descInx = specCollSubCreate( rsComm, dataObjInp );
    }
    else {

        /* dataObj exist */
        if ( getValByKey( &dataObjInp->condInput, FORCE_FLAG_KW ) != nullptr ) {
            dataObjInp->openFlags |= O_TRUNC | O_RDWR;
            // =-=-=-=-=-=-=-
            // re-determine the resource hierarchy since this is an open instead of a create
            std::string       hier;
            irods::error ret = irods::resolve_resource_hierarchy( irods::WRITE_OPERATION,
                               rsComm, dataObjInp, hier );
            if ( !ret.ok() ) {
                if ( HIERARCHY_ERROR == ret.code() ) {
                    char* dst_resc_kw = getValByKey( &dataObjInp->condInput, DEST_RESC_NAME_KW );
                    rodsLogAndErrorMsg(
                        LOG_ERROR,
                        &rsComm->rError,
                        status,
                        "Cannot overwrite replica of [%s] to resource [%s] as no prior replica exists on that resource",
                        dataObjInp->objPath,
                        dst_resc_kw );
                }
                else {
                    std::stringstream msg;
                    msg << "failed in irods::resolve_resource_hierarchy for [";
                    msg << dataObjInp->objPath << "]";
                    irods::log( PASSMSG( msg.str(), ret ) );
                }
                freeRodsObjStat( rodsObjStatOut );

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
            l1descInx = rsDataObjOpen( rsComm, dataObjInp );

        }
        else {
            l1descInx = OVERWRITE_WITHOUT_FORCE_FLAG;
        }
    }


    freeRodsObjStat( rodsObjStatOut );

    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    if ( lockFd >= 0 ) {
        if ( l1descInx >= 0 ) {
            L1desc[l1descInx].lockFd = lockFd;
        }
        else {
            char fd_string[NAME_LEN];
            snprintf( fd_string, sizeof( fd_string ), "%-d", lockFd );
            addKeyVal( &dataObjInp->condInput, LOCK_FD_KW, fd_string );
            irods::server_api_call(
                DATA_OBJ_UNLOCK_AN,
                rsComm,
                dataObjInp,
                nullptr,
                ( void** ) nullptr,
                nullptr );
        }
    }

    // =-=-=-=-=-=-=-
    return l1descInx;
}

int
_rsDataObjCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status = 0;
    int l1descInx = 0;

    status = l1descInx = _rsDataObjCreateWithResc(
                             rsComm,
                             dataObjInp,
                             "INVALID_RESOURCE_NAME");

    // JMC - legacy resource - if (status < 0) {
    if ( status >= 0 ) {
        return status;
    }
    else {
        rodsLog( LOG_NOTICE,
                 "rsDataObjCreate: Internal error" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
}

int
specCollSubCreate( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status;
    int l1descInx;
    dataObjInfo_t *dataObjInfo = nullptr;

    status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                    WRITE_COLL_PERM, 0, &dataObjInfo );
    if ( dataObjInfo == nullptr ) { // JMC cppcheck
        rodsLog( LOG_ERROR, "specCollSubCreate :: dataObjInp is null" );
        return status;
    }
    if ( status >= 0 ) {
        rodsLog( LOG_ERROR,
                 "specCollSubCreate: phyPath %s already exist",
                 dataObjInfo->filePath );
        freeDataObjInfo( dataObjInfo );
        return SYS_COPY_ALREADY_IN_RESC;
    }
    else if ( status != SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
        return status;
    }

    l1descInx = allocL1desc();

    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    dataObjInfo->replStatus = NEWLY_CREATED_COPY;
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize );

    if ( getValByKey( &dataObjInp->condInput, NO_OPEN_FLAG_KW ) == nullptr ) {
        status = dataCreate( rsComm, l1descInx );
        if ( status < 0 ) {
            freeL1desc( l1descInx );
            return status;
        }
    }

    return l1descInx;
}

/* _rsDataObjCreateWithResc - Create a single copy of the data Object
 * given a resource name.
 *
 * return l1descInx.
 */

int
_rsDataObjCreateWithResc(
    rsComm_t*     rsComm,
    dataObjInp_t* dataObjInp,
    const std::string&   _resc_name ) {

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
    // stage kvp for passthru
    char* kvp_str = getValByKey(
                        &dataObjInp->condInput,
                        KEY_VALUE_PASSTHROUGH_KW );
    if ( kvp_str ) {
        addKeyVal(
            &dataObjInfo->condInput,
            KEY_VALUE_PASSTHROUGH_KW,
            kvp_str );

    }

    // =-=-=-=-=-=-=-
    // honor the purge flag
    if ( getValByKey( &dataObjInp->condInput, PURGE_CACHE_KW ) != nullptr ) { // JMC - backport 4537
        L1desc[l1descInx].purgeCacheFlag = 1;
    }
    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( resc_hier ) {
        // we need to favor the results from the PEP acSetRescSchemeForCreate
        irods::hierarchy_parser parse;
        parse.set_string( resc_hier );

        std::string root;
        parse.first_resc( root );

        std::string leaf;
        parse.last_resc( leaf );

        rstrcpy( dataObjInfo->rescName, root.c_str(), NAME_LEN );
        rstrcpy( dataObjInfo->rescHier, resc_hier, MAX_NAME_LEN );
    }
    else {
        rstrcpy( dataObjInfo->rescName, _resc_name.c_str(), NAME_LEN );// backwards compatibility
        rstrcpy( dataObjInfo->rescHier, _resc_name.c_str(), MAX_NAME_LEN );// backwards compatibility
    }

    irods::error ret = resc_mgr.hier_to_leaf_id(dataObjInfo->rescHier, dataObjInfo->rescId );
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    dataObjInfo->replStatus = NEWLY_CREATED_COPY; // JMC - backport 4754
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize );

    status = getFilePathName( rsComm, dataObjInfo, L1desc[l1descInx].dataObjInp );

    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return status;
    }

    /* output of _rsDataObjCreate - filePath stored in
     * dataObjInfo struct */

    if ( getValByKey( &dataObjInp->condInput, NO_OPEN_FLAG_KW ) != nullptr ) {

        /* don't actually physically open the file */
        status = 0;
    }
    else {

        status = dataObjCreateAndReg( rsComm, l1descInx );
    }

    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return status;
    }
    else {
        return l1descInx;
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
        return status;
    }

    /* only register new copy */
    status = svrRegDataObj( rsComm, myDataObjInfo );
    if ( status < 0 ) {
        l3Unlink( rsComm, myDataObjInfo );
        rodsLog( LOG_NOTICE,
                 "dataObjCreateAndReg: rsRegDataObj for %s failed, status = %d",
                 myDataObjInfo->objPath, status );
        return status;
    }
    else {
        myDataObjInfo->replNum = status;
        return 0;
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
        return status;
    }
    else {
        L1desc[l1descInx].l3descInx = status;
    }

    return 0;
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
        irods::log( PASSMSG( "l3Create - failed in get_loc_for_hier_string", ret ) );
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

    return l3descInx;
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
        irods::log( PASSMSG( "l3CreateByObjInfo - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }


    fileCreateInp_t fileCreateInp;
    memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );
    rstrcpy( fileCreateInp.resc_name_, location.c_str(),      MAX_NAME_LEN );
    rstrcpy( fileCreateInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileCreateInp.objPath,    dataObjInfo->objPath,  MAX_NAME_LEN );
    rstrcpy( fileCreateInp.addr.hostAddr, location.c_str(), NAME_LEN );
    copyKeyVal(
        &dataObjInfo->condInput,
        &fileCreateInp.condInput );

    rstrcpy( fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    fileCreateInp.mode = getFileMode( dataObjInp );
    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    chkType = getchkPathPerm( rsComm, dataObjInp, dataObjInfo );
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
        fileCreateOut_t* create_out = nullptr;
        l3descInx = rsFileCreate( rsComm, &fileCreateInp, &create_out );

        // update the dataObjInfo with the potential changes made by the resource - hcj
        if ( create_out != nullptr ) {
            rstrcpy( dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN );
            rstrcpy( dataObjInfo->filePath, create_out->file_name, MAX_NAME_LEN );
            free( create_out );
        }

        //update the filename in case of a retry
        rstrcpy( fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
        retryCnt++;
    }
    while ( getErrno( l3descInx ) == EEXIST &&
            resolveDupFilePath( rsComm, dataObjInfo, dataObjInp ) >= 0 &&
            l3descInx <= 2 && retryCnt < 100 );
    clearKeyVal( &fileCreateInp.condInput );
    return l3descInx;
}

/* getRescForCreate - given the resource input in dataObjInp,
 * fills out _resc_name after applying the acSetRescSchemeForCreate
 * rule.
 * Return 1 of the "random" sorting scheme is used. Otherwise return 0
 * or an error code.
 */

int getRescForCreate(
    rsComm_t*     _comm,
    dataObjInp_t* _obj_inp,
    std::string&  _resc_name ) {

    /* query rcat for resource info and sort it */
    ruleExecInfo_t rei;
    initReiWithDataObjInp( &rei, _comm, _obj_inp );

    int status = 0;
    if ( _obj_inp->oprType == REPLICATE_OPR ) {
        status = applyRule( "acSetRescSchemeForRepl", nullptr, &rei, NO_SAVE_REI );
    }
    else {
        status = applyRule( "acSetRescSchemeForCreate", nullptr, &rei, NO_SAVE_REI );
    }
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);

    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }

        rodsLog(
            LOG_NOTICE,
            "getRescForCreate:acSetRescSchemeForCreate error for %s,status=%d",
            _obj_inp->objPath,
            status );

        return status;
    }

    // get resource name
    if ( !strlen( rei.rescName ) ) {
        irods::error set_err = irods::set_default_resource(
                                   _comm,
                                   "", "",
                                   &_obj_inp->condInput,
                                   _resc_name );
        if ( !set_err.ok() ) {
            irods::log( PASS( set_err ) );
            return SYS_INVALID_RESC_INPUT;
        }
    }
    else {
        _resc_name = rei.rescName;
    }

    status = setRescQuota(
                 _comm,
                 _obj_inp->objPath,
                 _resc_name.c_str(),
                 _obj_inp->dataSize );
    if( status == SYS_RESC_QUOTA_EXCEEDED ) {
        return SYS_RESC_QUOTA_EXCEEDED;
    }


    return 0;
}
