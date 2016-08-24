/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getTempPassword.h for a description of this API call.*/

#include "getTempPassword.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsGetTempPassword.hpp"

int
rsGetTempPassword( rsComm_t *rsComm,
                   getTempPasswordOut_t **getTempPasswordOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );

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

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
             status = _rsGetTempPassword( rsComm, getTempPasswordOut );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
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
        status = rcGetTempPassword( rodsServerHost->conn,
                                    getTempPasswordOut );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGetTempPassword: rcGetTempPassword failed, status = %d",
                 status );
    }
    return status;
}

int
_rsGetTempPassword( rsComm_t *rsComm,
                    getTempPasswordOut_t **getTempPasswordOut ) {
    int status;
    getTempPasswordOut_t *myGetTempPasswordOut;

    myGetTempPasswordOut = ( getTempPasswordOut_t* )malloc( sizeof( getTempPasswordOut_t ) );

    status = chlMakeTempPw( rsComm,
                            myGetTempPasswordOut->stringToHashWith, "" );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "_rsGetTempPassword: getTempPassword, status = %d",
                 status );
    }

    *getTempPasswordOut = myGetTempPasswordOut;

    return status;
}
