/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See specificQuery.h for a description of this API call.*/

#include "specificQuery.h"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.h"
#include "rcMisc.h"
#include "irods_log.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsSpecificQuery.hpp"

int
rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp,
                 genQueryOut_t **genQueryOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;
    char *zoneHint = "";

    /*  zoneHint = getZoneHintForGenQuery (genQueryInp); (need something like this?) */
    zoneHint = getValByKey( &specificQueryInp->condInput, ZONE_KW );

    status = getAndConnRcatHost( rsComm, SLAVE_RCAT, ( const char* )zoneHint,
                                 &rodsServerHost );
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
            status = _rsSpecificQuery( rsComm, specificQueryInp, genQueryOut );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            rodsLog( LOG_NOTICE,
                     "rsSpecificQuery error. RCAT is not configured on this host" );
            return SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        status = rcSpecificQuery( rodsServerHost->conn,
                                  specificQueryInp, genQueryOut );
    }
    if ( status < 0  && status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "rsSpecificQuery: rcSpecificQuery failed, status = %d", status );
    }
    return status;
}

int
_rsSpecificQuery( rsComm_t *rsComm, specificQueryInp_t *specificQueryInp,
                  genQueryOut_t **genQueryOut ) {
    int status;

    *genQueryOut = ( genQueryOut_t* )malloc( sizeof( genQueryOut_t ) );
    memset( ( char * )*genQueryOut, 0, sizeof( genQueryOut_t ) );

    status = chlSpecificQuery( *specificQueryInp, *genQueryOut );

    if ( status == CAT_UNKNOWN_SPECIFIC_QUERY ) {
        int i;
        i = addRErrorMsg( &rsComm->rError, 0, "The SQL is not pre-defined.\n  See 'iadmin h asq' (add specific query)" );
        if ( i < 0 ) {
            irods::log( i, "addErrorMsg failed" );
        }
    }

    if ( status < 0 ) {
        clearGenQueryOut( *genQueryOut );
        free( *genQueryOut );
        *genQueryOut = NULL;
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "_rsSpecificQuery: specificQuery status = %d", status );
        }
        return status;
    }
    return status;
}
