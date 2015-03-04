/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See simpleQuery.h for a description of this API call.*/

#include "simpleQuery.hpp"
#include "rodsConnect.h"
#include "icatHighLevelRoutines.hpp"

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
#ifdef RODS_CAT
        status = _rsSimpleQuery( rsComm, simpleQueryInp, simpleQueryOut );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
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

#ifdef RODS_CAT
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
#endif
