/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataPut.h for a description of this API call.*/

#include "dataPut.h"
#include "rodsLog.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

/* rsDataPut - this routine setup portalOprOut with the resource server
 * for parallel put operation.
 */

int
rsDataPut (rsComm_t *rsComm, dataOprInp_t *dataOprInp, 
portalOprOut_t **portalOprOut)
{
    int status;
    int remoteFlag;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;

    l3descInx = dataOprInp->destL3descInx;

    if (getValByKey (&dataOprInp->condInput, EXEC_LOCALLY_KW) != NULL) {
	remoteFlag = LOCAL_HOST;
    } else {
        rodsServerHost = FileDesc[l3descInx].rodsServerHost;
	if (rodsServerHost == NULL) {
	    rodsLog (LOG_NOTICE, "rsDataPut: NULL rodsServerHost");
	    return (SYS_INTERNAL_NULL_INPUT_ERR);
	}
        remoteFlag = rodsServerHost->localFlag;
    }

    if (remoteFlag == LOCAL_HOST) {
        status = _rsDataPut (rsComm, dataOprInp, portalOprOut);
    } else {
	addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
        status = remoteDataPut (rsComm, dataOprInp, portalOprOut,
         rodsServerHost);
        clearKeyVal (&dataOprInp->condInput);
    }


    return (status);
}

int
_rsDataPut (rsComm_t *rsComm, dataOprInp_t *dataOprInp,
portalOprOut_t **portalOprOut)
{
    int status;

    status = setupSrvPortalForParaOpr (rsComm, dataOprInp, PUT_OPR,
      portalOprOut);

    return status;
#if 0
    portalOprOut_t *myDataObjPutOut;
    int portalSock;
    int proto;

#ifdef RBUDP_TRANSFER
    if (getValByKey (&dataOprInp->condInput, RBUDP_TRANSFER_KW) != NULL) {
	proto = SOCK_DGRAM;
    } else {
	proto = SOCK_STREAM;
    }
#else
    proto = SOCK_STREAM;
#endif  /* RBUDP_TRANSFER */

    myDataObjPutOut = (portalOprOut_t *) malloc (sizeof (portalOprOut_t));
    memset (myDataObjPutOut, 0, sizeof (portalOprOut_t));

    *portalOprOut = myDataObjPutOut;

    if (getValByKey (&dataOprInp->condInput, STREAMING_KW) != NULL ||
      proto == SOCK_DGRAM) {
	/* streaming or udp - use only one thread */
	myDataObjPutOut->numThreads = 1;
    } else {
        myDataObjPutOut->numThreads =
          myDataObjPutOut->numThreads = getNumThreads (rsComm,
	   dataOprInp->dataSize, dataOprInp->numThreads, 
	   &dataOprInp->condInput);
    }

    if (myDataObjPutOut->numThreads == 0) {
        return 0;
    } else {
        portalOpr_t *myPortalOpr;

        /* setup the portal */
        portalSock = createSrvPortal (rsComm, &myDataObjPutOut->portList,
	  proto);
        if (portalSock < 0) {
            rodsLog (LOG_NOTICE,
              "_rsDataPut: createSrvPortal error, ststus = %d", portalSock);
              myDataObjPutOut->status = portalSock;
              return portalSock;
        }
        myPortalOpr = rsComm->portalOpr =
          (portalOpr_t *) malloc (sizeof (portalOpr_t));
        myPortalOpr->oprType = PUT_OPR;
        myPortalOpr->portList = myDataObjPutOut->portList;
	myPortalOpr->dataOprInp = *dataOprInp;
	memset (&dataOprInp->condInput, 0, sizeof (dataOprInp->condInput));
        myPortalOpr->dataOprInp.numThreads = myDataObjPutOut->numThreads;
    }
    return (0);
#endif
}


int
remoteDataPut (rsComm_t *rsComm, dataOprInp_t *dataOprInp, 
portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDataPut: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    dataOprInp->destL3descInx = convL3descInx (dataOprInp->destL3descInx);
 
    status = rcDataPut (rodsServerHost->conn, dataOprInp, portalOprOut);

    return (status);
}

