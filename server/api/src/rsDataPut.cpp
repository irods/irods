#include "irods/rsDataPut.hpp"

#include "irods/dataObjInpOut.h"
#include "irods/dataPut.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rcConnect.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/rodsLog.h"
#include "irods/rsGlobalExtern.hpp"

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
        status = setupSrvPortalForParaOpr(rsComm, dataOprInp, PUT_OPR, portalOprOut);
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
