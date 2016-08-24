/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getLimitedPassword.h for a description of this API call.*/

#include "getLimitedPassword.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsGetLimitedPassword.hpp"



int
rsGetLimitedPassword( rsComm_t *rsComm,
                      getLimitedPasswordInp_t *getLimitedPasswordInp,
                      getLimitedPasswordOut_t **getLimitedPasswordOut ) {
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
            status = _rsGetLimitedPassword(
                         rsComm,
                         getLimitedPasswordInp,
                         getLimitedPasswordOut );
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
        status = rcGetLimitedPassword( rodsServerHost->conn,
                                       getLimitedPasswordInp,
                                       getLimitedPasswordOut );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGetLimitedPassword: rcGetLimitedPassword failed, status = %d",
                 status );
    }
    return status;
}

int
_rsGetLimitedPassword( rsComm_t *rsComm,
                       getLimitedPasswordInp_t *getLimitedPasswordInp,
                       getLimitedPasswordOut_t **getLimitedPasswordOut ) {
    int status;
    getLimitedPasswordOut_t *myGetLimitedPasswordOut;

    myGetLimitedPasswordOut = ( getLimitedPasswordOut_t* )malloc( sizeof( getLimitedPasswordOut_t ) );

    status = chlMakeLimitedPw( rsComm, getLimitedPasswordInp->ttl,
                               myGetLimitedPasswordOut->stringToHashWith );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "_rsGetLimitedPassword: getLimitedPassword, status = %d",
                 status );
    }

    *getLimitedPasswordOut = myGetLimitedPasswordOut;

    return status;
}
