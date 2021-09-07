#include "syncMountedColl.h"
#include "rodsLog.h"
#include "icatDefines.h"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "resource.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsSyncMountedColl.hpp"
#include "miscServerFunct.hpp"
#include "apiHeaderAll.h"
#include "rsStructFileSync.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"

#include <cstring>

int
rsSyncMountedColl( rsComm_t *rsComm, dataObjInp_t *syncMountedCollInp ) {
    rodsLog( LOG_NOTICE, "rsSyncMountedColl - start" );

    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;
    dataObjInp_t myDataObjInp;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    status = collStat( rsComm, syncMountedCollInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) {
        freeRodsObjStat( rodsObjStatOut );
        return status;    // JMC cppcheck - nullptr
    }

    if ( rodsObjStatOut->specColl == NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "rsSyncMountedColl: %s not a mounted collection",
                 syncMountedCollInp->objPath );
        return SYS_COLL_NOT_MOUNTED_ERR;
    }

    std::memset(&myDataObjInp, 0, sizeof(myDataObjInp));
    rstrcpy( myDataObjInp.objPath, rodsObjStatOut->specColl->objPath,
             MAX_NAME_LEN );
    remoteFlag = getAndConnRemoteZone( rsComm, &myDataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        freeRodsObjStat( rodsObjStatOut );
        return remoteFlag;
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

    return status;
}

int
_rsSyncMountedColl( rsComm_t *rsComm, specColl_t *specColl, int oprType ) {
    int status;

    if ( getStructFileType( specColl ) >= 0 ) { 	/* a struct file */
        structFileOprInp_t structFileOprInp;

        if ( strlen( specColl->resource ) == 0 ) {
            /* nothing to sync */
            return 0;
        }

        memset( &structFileOprInp, 0, sizeof( structFileOprInp ) );

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( specColl->rescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "rsSyncMountedColl - failed in get_loc_for_hier_string", ret ) );
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

    return status;
}
