/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "regReplica.h"
#include "icatHighLevelRoutines.h"

int
rsRegReplica (rsComm_t *rsComm, regReplica_t *regReplicaInp)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *srcDataObjInfo;
    dataObjInfo_t *destDataObjInfo;

    srcDataObjInfo = regReplicaInp->srcDataObjInfo;
    destDataObjInfo = regReplicaInp->destDataObjInfo;

    status = getAndConnRcatHost (rsComm, MASTER_RCAT, srcDataObjInfo->objPath,
      &rodsServerHost);
    if (status < 0) {
       return(status);
    }
    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
        status = _rsRegReplica (rsComm, regReplicaInp);
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } else {
        status = rcRegReplica (rodsServerHost->conn, regReplicaInp);
	if (status >= 0) regReplicaInp->destDataObjInfo->replNum = status;

    }

    return (status);
}

int
_rsRegReplica (rsComm_t *rsComm, regReplica_t *regReplicaInp)
{
#ifdef RODS_CAT
    int status;
    dataObjInfo_t *srcDataObjInfo;
    dataObjInfo_t *destDataObjInfo;
    int savedClientAuthFlag;

    srcDataObjInfo = regReplicaInp->srcDataObjInfo;
    destDataObjInfo = regReplicaInp->destDataObjInfo;

    if (getValByKey (&regReplicaInp->condInput, SU_CLIENT_USER_KW) != NULL) {
	savedClientAuthFlag = rsComm->clientUser.authInfo.authFlag;
	rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
        status = chlRegReplica (rsComm, srcDataObjInfo, destDataObjInfo,
          &regReplicaInp->condInput);
	/* restore it */
	rsComm->clientUser.authInfo.authFlag = savedClientAuthFlag;
    } else {
        status = chlRegReplica (rsComm, srcDataObjInfo, destDataObjInfo,
          &regReplicaInp->condInput);
	if (status >= 0) status = destDataObjInfo->replNum;
    }
    return (status);
#else
    return (SYS_NO_RCAT_SERVER_ERR);
#endif
}

