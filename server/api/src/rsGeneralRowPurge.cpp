/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See generalRowPurge.h for a description of this API call.*/

#include "generalRowPurge.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsGeneralRowPurge.hpp"

int
rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    rodsLog( LOG_DEBUG, "generalRowPurge" );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )NULL, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {

        std::string svc_role;
        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsGeneralRowPurge( rsComm, generalRowPurgeInp );
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
        status = rcGeneralRowPurge( rodsServerHost->conn,
                                    generalRowPurgeInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsGeneralRowPurge: rcGeneralRowPurge failed" );
    }
    return status;
}

int
_rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp ) {
    int status;

    rodsLog( LOG_DEBUG,
             "_rsGeneralRowPurge tableName=%s",
             generalRowPurgeInp->tableName );

    if ( strcmp( generalRowPurgeInp->tableName, "serverload" ) == 0 ) {
        status = chlPurgeServerLoad( rsComm,
                                     generalRowPurgeInp->secondsAgo );
        return status;
    }
    if ( strcmp( generalRowPurgeInp->tableName, "serverloaddigest" ) == 0 ) {
        status = chlPurgeServerLoadDigest( rsComm,
                                           generalRowPurgeInp->secondsAgo );
        return status;
    }
    return CAT_INVALID_ARGUMENT;
}
