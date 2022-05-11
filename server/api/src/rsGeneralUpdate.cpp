/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See generalUpdate.h for a description of this API call.*/

#include "irods/generalUpdate.h"
#include "irods/rsGeneralUpdate.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_configuration_keywords.hpp"

int
rsGeneralUpdate( rsComm_t *rsComm, generalUpdateInp_t *generalUpdateInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "generalUpdate" );

    status = getAndConnRcatHost( rsComm, PRIMARY_RCAT, ( const char* )NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsGeneralUpdate( generalUpdateInp );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        status = rcGeneralUpdate( rodsServerHost->conn,
                                  generalUpdateInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGeneralUpdate: rcGeneralUpdate failed" );
    }
    return status;
}

int
_rsGeneralUpdate( generalUpdateInp_t *generalUpdateInp ) {
    int status;

    status  = chlGeneralUpdate( *generalUpdateInp );

    return status;
}
