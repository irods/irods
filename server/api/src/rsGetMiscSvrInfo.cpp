/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGetMiscSvrInfo.c - server routine that handles the the GetMiscSvrInfo
 * API
 */

/* script generated code */
#include "getMiscSvrInfo.h"
#include "irods_server_properties.hpp"
#include "irods_log.hpp"
#include "rodsVersion.h"
#include "miscServerFunct.hpp"
#include "rsGetMiscSvrInfo.hpp"

int
rsGetMiscSvrInfo( rsComm_t *rsComm, miscSvrInfo_t **outSvrInfo ) {
    if ( !rsComm || !outSvrInfo ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    miscSvrInfo_t *myOutSvrInfo;
    char *tmpStr;

    myOutSvrInfo = *outSvrInfo = ( miscSvrInfo_t* )malloc( sizeof( miscSvrInfo_t ) );

    memset( myOutSvrInfo, 0, sizeof( miscSvrInfo_t ) );

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        myOutSvrInfo->serverType = RCAT_ENABLED;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        myOutSvrInfo->serverType = RCAT_NOT_ENABLED;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    rstrcpy( myOutSvrInfo->relVersion, RODS_REL_VERSION, NAME_LEN );
    rstrcpy( myOutSvrInfo->apiVersion, RODS_API_VERSION, NAME_LEN );

    const auto& zone_name = irods::get_server_property<const std::string>(irods::CFG_ZONE_NAME);
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    snprintf( myOutSvrInfo->rodsZone, NAME_LEN, "%s", zone_name.c_str() );
    if ( ( tmpStr = getenv( SERVER_BOOT_TIME ) ) != NULL ) {
        myOutSvrInfo->serverBootTime = atoi( tmpStr );
    }

    return 0;
}
