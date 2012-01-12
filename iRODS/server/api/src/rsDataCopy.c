/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataCopy.h for a description of this API call.*/

#include "dataCopy.h"
#include "rcPortalOpr.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

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
rsDataCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    int status;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;
    dataOprInp_t *dataOprInp;

    dataOprInp = &dataCopyInp->dataOprInp;


    if (getValByKey (&dataOprInp->condInput, EXEC_LOCALLY_KW) != NULL ||
      dataCopyInp->portalOprOut.numThreads == 0) {
	/* XXXXX do it locally if numThreads == 0 */
        status = _rsDataCopy (rsComm, dataCopyInp);
    } else {
        if (dataOprInp->destL3descInx > 0) {
            l3descInx = dataOprInp->destL3descInx;
	} else {
            l3descInx = dataOprInp->srcL3descInx;
        }
	rodsServerHost = FileDesc[l3descInx].rodsServerHost;
	if (rodsServerHost != NULL && rodsServerHost->localFlag != LOCAL_HOST) {
	    addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
            status = remoteDataCopy (rsComm, dataCopyInp, rodsServerHost);
            clearKeyVal (&dataOprInp->condInput);
	} else {
            status = _rsDataCopy (rsComm, dataCopyInp);
	}
    }
    return (status);
}

int
remoteDataCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp,
rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDataCopy: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    dataCopyInp->dataOprInp.destL3descInx = 
      convL3descInx (dataCopyInp->dataOprInp.destL3descInx);

    status = rcDataCopy (rodsServerHost->conn, dataCopyInp);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
         "remoteDataCopy: rcDataCopy failed");
    }

    return status;
}

int
_rsDataCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    dataOprInp_t *dataOprInp;
    int retVal;

    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
	  "_rsDataCopy: NULL dataCopyInp input");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    } 

    dataOprInp = &dataCopyInp->dataOprInp;
    if (dataOprInp->oprType == SAME_HOST_COPY_OPR) {
	/* src is on the same host */
	retVal = sameHostCopy (rsComm, dataCopyInp);
    } else if (dataOprInp->oprType == COPY_TO_LOCAL_OPR || 
      dataOprInp->oprType == COPY_TO_REM_OPR) {
	retVal = remLocCopy (rsComm, dataCopyInp);
    } else {
	rodsLog (LOG_NOTICE,
          "_rsDataCopy: Invalid oprType %d", dataOprInp->oprType);
	return (SYS_INVALID_OPR_TYPE);
    }

    return (retVal);
}

