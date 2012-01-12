/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataGet.h for a description of this API call.*/

#include "dataGet.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

/* rsDataGet - this routine setup portalOprOut with the resource server
 * for parallel get operation.
 */

int
rsDataGet (rsComm_t *rsComm, dataOprInp_t *dataOprInp, 
portalOprOut_t **portalOprOut)
{
    int status;
    int remoteFlag;
    int l3descInx;
    rodsServerHost_t *rodsServerHost;

    l3descInx = dataOprInp->srcL3descInx;

    if (getValByKey (&dataOprInp->condInput, EXEC_LOCALLY_KW) != NULL) {
        remoteFlag = LOCAL_HOST;
    } else {
        rodsServerHost = FileDesc[l3descInx].rodsServerHost;
        if (rodsServerHost == NULL) {
            rodsLog (LOG_NOTICE, "rsDataGet: NULL rodsServerHost");
            return (SYS_INTERNAL_NULL_INPUT_ERR);
        }
        remoteFlag = rodsServerHost->localFlag;
    }

    if (remoteFlag == LOCAL_HOST) {
        status = _rsDataGet (rsComm, dataOprInp, portalOprOut);
    } else {
        addKeyVal (&dataOprInp->condInput, EXEC_LOCALLY_KW, "");
        status = remoteDataGet (rsComm, dataOprInp, portalOprOut,
         rodsServerHost);
        clearKeyVal (&dataOprInp->condInput);
    }


    return (status);
}

int
_rsDataGet (rsComm_t *rsComm, dataOprInp_t *dataOprInp,
portalOprOut_t **portalOprOut)
{
    int status;

    status = setupSrvPortalForParaOpr (rsComm, dataOprInp, GET_OPR,
      portalOprOut);

    return status;
#if 0
   portalOprOut_t *myDataObjGetOut;
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

    myDataObjGetOut = (portalOprOut_t *) malloc (sizeof (portalOprOut_t));
    memset (myDataObjGetOut, 0, sizeof (portalOprOut_t));

    *portalOprOut = myDataObjGetOut;

    if (getValByKey (&dataOprInp->condInput, STREAMING_KW) != NULL ||
      proto == SOCK_DGRAM) {
	/* streaming or udp - use only one thread */
        myDataObjGetOut->numThreads = 1;
    } else {
        myDataObjGetOut->numThreads =
          myDataObjGetOut->numThreads = getNumThreads (rsComm,
	  dataOprInp->dataSize, dataOprInp->numThreads,  
	  &dataOprInp->condInput);
    }

    
    if (myDataObjGetOut->numThreads == 0) {
        return 0;
    } else {
        portalOpr_t *myPortalOpr;

        /* setup the portal */
        portalSock = createSrvPortal (rsComm, &myDataObjGetOut->portList,
	  proto);
        if (portalSock < 0) {
            rodsLog (LOG_NOTICE,
              "_rsDataGet: createSrvPortal error, ststus = %d", portalSock);
              myDataObjGetOut->status = portalSock;
              return portalSock;
        }
        myPortalOpr = rsComm->portalOpr =
          (portalOpr_t *) malloc (sizeof (portalOpr_t));
        myPortalOpr->oprType = GET_OPR;
        myPortalOpr->portList = myDataObjGetOut->portList;
        myPortalOpr->dataOprInp = *dataOprInp;
        memset (&dataOprInp->condInput, 0, sizeof (dataOprInp->condInput));
        myPortalOpr->dataOprInp.numThreads = myDataObjGetOut->numThreads;
    }
    return (0);
#endif
}

int
remoteDataGet (rsComm_t *rsComm, dataOprInp_t *dataOprInp,
portalOprOut_t **portalOprOut, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteDataGet: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    dataOprInp->srcL3descInx = convL3descInx (dataOprInp->srcL3descInx);
    status = rcDataGet (rodsServerHost->conn, dataOprInp, portalOprOut);

    return (status);
}

