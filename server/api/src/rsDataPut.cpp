/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataPut.h for a description of this API call.*/

#include "rcMisc.h"
#include "dataPut.h"
#include "rodsLog.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsDataPut.hpp"

/* rsDataPut - this routine setup portalOprOut with the resource server
 * for parallel put operation.
 */

int
rsDataPut( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
           portalOprOut_t **portalOprOut ) {
    int status;
    int remoteFlag;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;

    l3descInx = dataOprInp->destL3descInx;

    if ( getValByKey( &dataOprInp->condInput, EXEC_LOCALLY_KW ) != NULL ) {
        remoteFlag = LOCAL_HOST;
    }
    else {
        rodsServerHost = FileDesc[l3descInx].rodsServerHost;
        if ( rodsServerHost == NULL ) {
            rodsLog( LOG_NOTICE, "rsDataPut: NULL rodsServerHost" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        remoteFlag = rodsServerHost->localFlag;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsDataPut( rsComm, dataOprInp, portalOprOut );
    }
    else {
        addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
        status = remoteDataPut( rsComm, dataOprInp, portalOprOut,
                                rodsServerHost );
        clearKeyVal( &dataOprInp->condInput );
    }


    return status;
}

int
_rsDataPut( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
            portalOprOut_t **portalOprOut ) {
    int status;

    status = setupSrvPortalForParaOpr( rsComm, dataOprInp, PUT_OPR,
                                       portalOprOut );

    return status;
}


int
remoteDataPut( rsComm_t *rsComm, dataOprInp_t *dataOprInp,
               portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteDataPut: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    dataOprInp->destL3descInx = convL3descInx( dataOprInp->destL3descInx );

    status = rcDataPut( rodsServerHost->conn, dataOprInp, portalOprOut );

    return status;
}
