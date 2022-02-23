/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See getHostForPut.h for a description of this API call.*/

#include "irods/getHostForPut.h"
#include "irods/rodsLog.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/dataObjCreate.h"
#include "irods/objMetaOpr.hpp"
#include "irods/resource.hpp"
#include "irods/collection.hpp"
#include "irods/specColl.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rsGetHostForPut.hpp"

// =-=-=-=-=-=-=-
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"


int rsGetHostForPut(
    rsComm_t*     rsComm,
    dataObjInp_t* dataObjInp,
    char **       outHost ) {
    rodsServerHost_t *rodsServerHost;
    const int remoteFlag = getAndConnRemoteZone(rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (REMOTE_HOST == remoteFlag) {
        const int status = rcGetHostForPut(rodsServerHost->conn, dataObjInp, outHost);
        if (status < 0) {
            return status;
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // working on the "home zone", determine if we need to redirect to a different
        // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
        // we know that the redirection decision has already been made
        std::string hier{};
        if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
            try {
                auto result = irods::resolve_resource_hierarchy(irods::CREATE_OPERATION, rsComm, *dataObjInp);
                hier = std::get<std::string>(result);
            }
            catch (const irods::exception& e ) {
                irods::log(e);
                return e.code();
            }
            addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );
        } // if keyword

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "rsGetHostForPut - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        // =-=-=-=-=-=-=-
        // set the out variable
        *outHost = strdup( location.c_str() );
    }

    return 0;

}
