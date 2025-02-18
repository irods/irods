#include "irods/rsDataGet.hpp"

#include "irods/dataGet.h"
#include "irods/dataObjInpOut.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/rodsConnect.h"
#include "irods/rsGlobalExtern.hpp"

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
        status = setupSrvPortalForParaOpr(rsComm, dataOprInp, GET_OPR, portalOprOut);
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
