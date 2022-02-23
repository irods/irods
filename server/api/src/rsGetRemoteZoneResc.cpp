#include "irods/getRemoteZoneResc.h"
#include "irods/rodsLog.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/physPath.hpp"
#include "irods/regDataObj.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rsGetRemoteZoneResc.hpp"
#include "irods/dataObjCreate.h"
#include "irods/dataObjOpen.h"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_resource_backport.hpp"

#include <cstring>

int
rsGetRemoteZoneResc( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                     rodsHostAddr_t **rescAddr ) {

    irods::error err = SUCCESS();

    // =-=-=-=-=-=-=-
    // acquire the operation requested
    char* remoteOprType = getValByKey(
                              &dataObjInp->condInput,
                              REMOTE_ZONE_OPR_KW );
    if ( !remoteOprType ) {
        rodsLog( LOG_ERROR,
                 "rsGetRemoteZoneResc: REMOTE_ZONE_OPR_KW not defined for %s",
                 dataObjInp->objPath );
        return USER_BAD_KEYWORD_ERR;
    }

    // =-=-=-=-=-=-=-
    // try to open the object in question, otherwise it is a create
    int status = 0;
    bool do_sort = false;  // only on open
    std::string oper = irods::CREATE_OPERATION;
    dataObjInfo_t *dataObjInfoHead = NULL;
    if ( strcmp( remoteOprType, REMOTE_OPEN ) == 0 ) {
        status = getDataObjInfoIncSpecColl(
                     rsComm,
                     dataObjInp,
                     &dataObjInfoHead );
        if ( status >= 0 ) {
            do_sort = true;
            oper = irods::OPEN_OPERATION;
        }
    }

    std::string hier{};
    try {
        auto result = irods::resolve_resource_hierarchy(oper, rsComm, *dataObjInp);
        hier = std::get<std::string>(result);
    }   
    catch (const irods::exception& e) { 
        irods::log(e);
        return e.code();
    }

    // =-=-=-=-=-=-=-
    // we resolved the hier str for subsequent api calls, etc.
    addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    // =-=-=-=-=-=-=-
    // we resolved the hier str for subsequent api calls, etc.
    std::string location;
    err = irods::get_loc_for_hier_string( hier, location );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
        return err.code();
    }

    // =-=-=-=-=-=-=-
    // if this was an open then run the sort and override the location
    if ( do_sort ) {
        int writeFlag;
        writeFlag = getWriteFlag( dataObjInp->openFlags );
        status = sortObjInfoForOpen(
                     &dataObjInfoHead,
                     &dataObjInp->condInput,
                     writeFlag );
        if ( status < 0 ) {
            rodsLog(
                LOG_ERROR,
                "rsGetRemoteZoneResc - sortObjInfoForOpen failed for [%s]",
                dataObjInp->objPath );
            return status;
        }

        // =-=-=-=-=-=-=-
        // extract resource location from hierarchy
        err = irods::get_resource_property<std::string>( dataObjInfoHead->rescId, irods::RESOURCE_LOCATION, location );
        if ( !err.ok() ) {
            irods::log( PASSMSG( "rsGetRemoteZoneResc - failed in get_resc_hier_property", err ) );
            return err.code();
        }
    }

    // =-=-=-=-=-=-=-
    // set the out variable
    *rescAddr = ( rodsHostAddr_t* )malloc( sizeof( rodsHostAddr_t ) );
    std::memset(*rescAddr, 0, sizeof(rodsHostAddr_t));
    rstrcpy( ( *rescAddr )->hostAddr, location.c_str(),       NAME_LEN );
    rstrcpy( ( *rescAddr )->zoneName, ZoneInfoHead->zoneName, NAME_LEN );

    return 0;

}
