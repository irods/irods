/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See getTempPasswordForOther.h for a description of this API call.*/

#include "getTempPasswordForOther.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsGetTempPasswordForOther.hpp"

int
rsGetTempPasswordForOther( rsComm_t *rsComm,
                           getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
                           getTempPasswordForOtherOut_t **getTempPasswordForOtherOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
    rodsLog( LOG_DEBUG,
             "rsGetTempPasswordForOther get stat=%d", status );
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
            status = _rsGetTempPasswordForOther(
                         rsComm,
                         getTempPasswordForOtherInp,
                         getTempPasswordForOtherOut );
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
        status = rcGetTempPasswordForOther( rodsServerHost->conn,
                                            getTempPasswordForOtherInp,
                                            getTempPasswordForOtherOut );
    }
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGetTempPasswordForOther: rcGetTempPasswordForOther failed, status = %d",
                 status );
    }
    return status;
}

int
_rsGetTempPasswordForOther( rsComm_t *rsComm,
                            getTempPasswordForOtherInp_t *getTempPasswordForOtherInp,
                            getTempPasswordForOtherOut_t **getTempPasswordForOtherOut ) {
    int status;
    getTempPasswordForOtherOut_t *myGetTempPasswordForOtherOut;

    myGetTempPasswordForOtherOut = ( getTempPasswordForOtherOut_t* )malloc( sizeof( getTempPasswordForOtherOut_t ) );

    status = chlMakeTempPw( rsComm,
                            myGetTempPasswordForOtherOut->stringToHashWith,
                            getTempPasswordForOtherInp->otherUser );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "_rsGetTempPasswordForOther: getTempPasswordForOther, status = %d",
                 status );
    }

    *getTempPasswordForOtherOut = myGetTempPasswordForOtherOut;

    return status;
}
