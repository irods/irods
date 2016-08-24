/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataGet.h for a description of this API call.*/

#include "rcMisc.h"
#include "dataGet.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsDataGet.hpp"

/* rsDataGet - this routine setup portalOprOut with the resource server
 * for parallel get operation.
 */

int
rsDataGet( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
           portalOprOut_t **portalOprOut ) {
    int status;
    int remoteFlag;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;

    l3descInx = dataOprInp->srcL3descInx;

    if ( getValByKey( &dataOprInp->condInput, EXEC_LOCALLY_KW ) != NULL ) {
        remoteFlag = LOCAL_HOST;
    }
    else {
        rodsServerHost = FileDesc[l3descInx].rodsServerHost;
        if ( rodsServerHost == NULL ) {
            rodsLog( LOG_NOTICE, "rsDataGet: NULL rodsServerHost" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        remoteFlag = rodsServerHost->localFlag;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsDataGet( rsComm, dataOprInp, portalOprOut );
    }
    else {
        addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
        status = remoteDataGet( rsComm, dataOprInp, portalOprOut,
                                rodsServerHost );
        clearKeyVal( &dataOprInp->condInput );
    }


    return status;
}

int
_rsDataGet( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
            portalOprOut_t **portalOprOut ) {
    return setupSrvPortalForParaOpr(
               rsComm,
               dataOprInp,
               GET_OPR,
               portalOprOut );

}

int
remoteDataGet( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
               portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteDataGet: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    dataOprInp->srcL3descInx = convL3descInx( dataOprInp->srcL3descInx );
    status = rcDataGet( rodsServerHost->conn, dataOprInp, portalOprOut );

    return status;
}
