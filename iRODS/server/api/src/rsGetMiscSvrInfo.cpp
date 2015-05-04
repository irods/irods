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

int
rsGetMiscSvrInfo( rsComm_t *rsComm, miscSvrInfo_t **outSvrInfo ) {
    if ( !rsComm || !outSvrInfo ) {
        return SYS_INVALID_INPUT_PARAM;
    }

    miscSvrInfo_t *myOutSvrInfo;
    char *tmpStr;

    myOutSvrInfo = *outSvrInfo = ( miscSvrInfo_t* )malloc( sizeof( miscSvrInfo_t ) );

    memset( myOutSvrInfo, 0, sizeof( miscSvrInfo_t ) );

    /* user writtened code */
#ifdef RODS_CAT
    myOutSvrInfo->serverType = RCAT_ENABLED;
#else
    myOutSvrInfo->serverType = RCAT_NOT_ENABLED;
#endif
    rstrcpy( myOutSvrInfo->relVersion, RODS_REL_VERSION, NAME_LEN );
    rstrcpy( myOutSvrInfo->apiVersion, RODS_API_VERSION, NAME_LEN );

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
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

