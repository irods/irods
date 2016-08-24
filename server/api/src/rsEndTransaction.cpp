/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See endTransaction.h for a description of this API call.*/

#include "endTransaction.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsEndTransaction.hpp"

int
rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "endTransaction" );

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
            status = _rsEndTransaction( rsComm, endTransactionInp );
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
        status = rcEndTransaction( rodsServerHost->conn,
                                   endTransactionInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsEndTransaction: rcEndTransaction failed" );
    }
    return status;
}

int
_rsEndTransaction( rsComm_t *rsComm, endTransactionInp_t *endTransactionInp ) {
    int status;

    rodsLog( LOG_DEBUG,
             "_rsEndTransaction arg0=%s",
             endTransactionInp->arg0 );

    if ( strcmp( endTransactionInp->arg0, "commit" ) == 0 ) {
        status = chlCommit( rsComm );
        return status;
    }

    if ( strcmp( endTransactionInp->arg0, "rollback" ) == 0 ) {
        status = chlRollback( rsComm );
        return status;
    }

    return CAT_INVALID_ARGUMENT;
}
