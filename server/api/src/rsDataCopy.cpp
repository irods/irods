/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataCopy.h for a description of this API call.*/

#include "rcMisc.h"
#include "dataCopy.h"
#include "rcPortalOpr.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsDataCopy.hpp"
#include "rodsConnect.h"
#include "irods_hierarchy_parser.hpp"
#include "irods_re_structs.hpp"

/* rsDataCopy - Do the copy data transfer.
 * Input -
 *   dataCopyInp_t dataCopyInp:
 *      dataOprInp:  - mostly setup by initDataOprInp()
 *         int oprType - SAME_HOST_COPY_OPR, COPY_TO_REM_OPR, COPY_TO_LOCAL_OPR
 *         int numThreads
 *         int srcL3descInx - the source L3descInx
 *         int destL3descInx - the target L3descInx
 *         int srcRescTypeInx;
 *         int destRescTypeInx;
 *         rodsLong_t offset
 *         rodsLong_t dataSize
 *         keyValPair_t condInput - valid input -
 *            EXEC_LOCALLY_KW - all operations except remote-remote copy
 *               have the keyword set.
 *
 *   portalOprOut_t portalOprOut - the resource server portal info.
 */
int
rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp ) {
    int status;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;
    dataOprInp_t *dataOprInp;

    dataOprInp = &dataCopyInp->dataOprInp;


    if ( getValByKey( &dataOprInp->condInput, EXEC_LOCALLY_KW ) != NULL ||
            dataCopyInp->portalOprOut.numThreads == 0 ) {
        /* XXXXX do it locally if numThreads == 0 */
        status = _rsDataCopy( rsComm, dataCopyInp );
    }
    else {
        if ( dataOprInp->destL3descInx > 0 ) {
            l3descInx = dataOprInp->destL3descInx;
        }
        else {
            l3descInx = dataOprInp->srcL3descInx;
        }
        rodsServerHost = FileDesc[l3descInx].rodsServerHost;
        if ( rodsServerHost != NULL && rodsServerHost->localFlag != LOCAL_HOST ) {
            addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
            status = remoteDataCopy( rsComm, dataCopyInp, rodsServerHost );
            clearKeyVal( &dataOprInp->condInput );
        }
        else {
            status = _rsDataCopy( rsComm, dataCopyInp );
        }
    }
    return status;
}

int
remoteDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp,
                rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteDataCopy: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    dataCopyInp->dataOprInp.srcL3descInx =
        convL3descInx( dataCopyInp->dataOprInp.srcL3descInx );
    dataCopyInp->dataOprInp.destL3descInx =
        convL3descInx( dataCopyInp->dataOprInp.destL3descInx );

    status = rcDataCopy( rodsServerHost->conn, dataCopyInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteDataCopy: rcDataCopy failed" );
    }

    return status;
}

static
int
apply_acPostProcForDataCopyReceived(rsComm_t *rsComm, dataOprInp_t *dataOprInp) {
    if (rsComm == NULL) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: NULL rsComm");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if (dataOprInp == NULL) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: NULL dataOprInp");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    const int l3_index = dataOprInp->destL3descInx;
    if (l3_index < 3 || l3_index >= NUM_FILE_DESC) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: bad l3 descriptor index %d", l3_index);
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    const char* resource_hierarchy = FileDesc[l3_index].rescHier;
    if (resource_hierarchy == NULL) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: NULL resource_hierarchy");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    irods::hierarchy_parser hierarchy_parser;
    irods::error err = hierarchy_parser.set_string(resource_hierarchy);
    if (!err.ok()) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: set_string error [%s]", err.result().c_str());
        return err.status();
    }

    std::string leaf_resource;
    err = hierarchy_parser.last_resc(leaf_resource);
    if (!err.ok()) {
        rodsLog(LOG_ERROR, "apply_acPostProcForDataCopyReceived: last_resc error [%s]", err.result().c_str());
        return err.status();
    }

    const char *args[] = {leaf_resource.c_str()};
    ruleExecInfo_t rei;
    memset(&rei, 0, sizeof(rei));
    rei.rsComm = rsComm;
    int ret = applyRuleArg("acPostProcForDataCopyReceived", args, sizeof(args)/sizeof(args[0]), &rei, NO_SAVE_REI);
    return ret;
}

int
_rsDataCopy( rsComm_t *rsComm, dataCopyInp_t *dataCopyInp ) {
    dataOprInp_t *dataOprInp;
    int retVal;

    if ( dataCopyInp == NULL ) {
        rodsLog( LOG_NOTICE,
                 "_rsDataCopy: NULL dataCopyInp input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    dataOprInp = &dataCopyInp->dataOprInp;
    if ( dataOprInp->oprType == SAME_HOST_COPY_OPR ) {
        /* src is on the same host */
        retVal = sameHostCopy( rsComm, dataCopyInp );
        if (retVal >= 0) {
            apply_acPostProcForDataCopyReceived(rsComm, dataOprInp);
        }
    }
    else if ( dataOprInp->oprType == COPY_TO_LOCAL_OPR ||
              dataOprInp->oprType == COPY_TO_REM_OPR ) {
        retVal = remLocCopy( rsComm, dataCopyInp );
        if (retVal >= 0 && dataOprInp->oprType == COPY_TO_LOCAL_OPR) {
            apply_acPostProcForDataCopyReceived(rsComm, dataOprInp);
        }
    }
    else {
        rodsLog( LOG_NOTICE,
                 "_rsDataCopy: Invalid oprType %d", dataOprInp->oprType );
        return SYS_INVALID_OPR_TYPE;
    }

    return retVal;
}
