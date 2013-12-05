/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See syncMountedColl.h for a description of this API call.*/

#include "syncMountedColl.hpp"
#include "rodsLog.hpp"
#include "icatDefines.hpp"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "resource.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "miscServerFunct.hpp"
#include "apiHeaderAll.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"

int
rsSyncMountedColl( rsComm_t *rsComm, dataObjInp_t *syncMountedCollInp ) {
    rodsLog( LOG_NOTICE, "rsSyncMountedColl - start" );

    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;
    dataObjInp_t myDataObjInp;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    status = collStat( rsComm, syncMountedCollInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) { return status; } // JMC cppcheck - nullptr

    if ( rodsObjStatOut->specColl == NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "rsSyncMountedColl: %s not a mounted collection",
                 syncMountedCollInp->objPath );
        return ( SYS_COLL_NOT_MOUNTED_ERR );
    }

    bzero( &myDataObjInp, sizeof( myDataObjInp ) );
    rstrcpy( myDataObjInp.objPath, rodsObjStatOut->specColl->objPath,
             MAX_NAME_LEN );
    remoteFlag = getAndConnRemoteZone( rsComm, &myDataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = rcSyncMountedColl( rodsServerHost->conn,
                                    syncMountedCollInp );
    }
    else {
        status = _rsSyncMountedColl( rsComm, rodsObjStatOut->specColl,
                                     syncMountedCollInp->oprType );
    }

    freeRodsObjStat( rodsObjStatOut );

    rodsLog( LOG_NOTICE, "rsSyncMountedColl - done" );

    return ( status );
}

int
_rsSyncMountedColl( rsComm_t *rsComm, specColl_t *specColl, int oprType ) {
    int status;

    if ( getStructFileType( specColl ) >= 0 ) { 	/* a struct file */
        structFileOprInp_t structFileOprInp;
        rescInfo_t         rescInfo;

        if ( strlen( specColl->resource ) == 0 ) {
            /* nothing to sync */
            return ( 0 );
        }

        memset( &structFileOprInp, 0, sizeof( structFileOprInp ) );
        /*status = resolveResc (specColl->resource, &rescInfo);
        if (status < 0) {
            rodsLog( LOG_NOTICE,"_rsSyncMountedColl: resolveResc error for %s, status = %d",
                     specColl->resource, status);
            return (status);
        }*/

        irods::error err = irods::get_resc_info( specColl->resource, rescInfo );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to resolve resource [";
            msg << specColl->resource;
            msg << "]";
            irods::log( PASSMSG( msg.str(), err ) );
        }

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( specColl->rescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "failed in get_loc_for_hier_String", ret ) );
            return -1;
        }


        addKeyVal( &structFileOprInp.condInput, RESC_HIER_STR_KW, specColl->rescHier );
        rstrcpy( structFileOprInp.addr.hostAddr, location.c_str(), NAME_LEN );
        structFileOprInp.oprType = oprType;
        structFileOprInp.specColl = specColl;
        status = rsStructFileSync( rsComm, &structFileOprInp );


    }
    else {			/* not a struct file */
        status = SYS_COLL_NOT_MOUNTED_ERR;
    }

    return ( status );
}

