/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See simpleQuery.h for a description of this API call.*/

#include "simpleQuery.h"
#include "rodsConnect.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsSimpleQuery.hpp"

int
rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp,
               simpleQueryOut_t **simpleQueryOut ) {
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
            status = _rsSimpleQuery( rsComm, simpleQueryInp, simpleQueryOut );
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
        status = rcSimpleQuery( rodsServerHost->conn,
                                simpleQueryInp, simpleQueryOut );
    }

    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "rsSimpleQuery: rcSimpleQuery failed, status = %d", status );
    }
    return status;
}

int
_rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp,
                simpleQueryOut_t **simpleQueryOut ) {
    int status;

    int control;

    int maxBufSize;
    char *outBuf;

    simpleQueryOut_t *myQueryOut;

    control = simpleQueryInp->control;

    maxBufSize = simpleQueryInp->maxBufSize;

    outBuf = ( char* )malloc( maxBufSize );

    status = chlSimpleQuery( rsComm, simpleQueryInp->sql,
                             simpleQueryInp->arg1,
                             simpleQueryInp->arg2,
                             simpleQueryInp->arg3,
                             simpleQueryInp->arg4,
                             simpleQueryInp->form,
                             &control, outBuf, maxBufSize );
    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "_rsSimpleQuery: simpleQuery for %s, status = %d",
                     simpleQueryInp->sql, status );
        }
        return status;
    }

    myQueryOut = ( simpleQueryOut_t* )malloc( sizeof( simpleQueryOut_t ) );
    myQueryOut->control = control;
    myQueryOut->outBuf = outBuf;

    *simpleQueryOut = myQueryOut;

    return status;
}
