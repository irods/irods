/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See specificQuery.h for a description of this API call.*/

#include "specificQuery.hpp"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.h"
#include "irods_log.hpp"

/* can be used for debug: */

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
#ifdef RODS_CAT
        status = _rsSpecificQuery( rsComm, specificQueryInp, genQueryOut );
#else
        rodsLog( LOG_NOTICE,
                 "rsSpecificQuery error. RCAT is not configured on this host" );
        return SYS_NO_RCAT_SERVER_ERR;
#endif
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

#ifdef RODS_CAT
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
#endif
