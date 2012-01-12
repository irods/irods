/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "unregDataObj.h"
#include "icatHighLevelRoutines.h"

int
rsUnregDataObj (rsComm_t *rsComm, unregDataObj_t *unregDataObjInp)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput; 

    condInput = unregDataObjInp->condInput;
    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = getAndConnRcatHost (rsComm, MASTER_RCAT, dataObjInfo->objPath,
      &rodsServerHost);
    if (status < 0) {
       return(status);
    }
    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
        status = _rsUnregDataObj (rsComm, unregDataObjInp);
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } else {
        status = rcUnregDataObj (rodsServerHost->conn, unregDataObjInp);
    }

    return (status);
}

int
_rsUnregDataObj (rsComm_t *rsComm, unregDataObj_t *unregDataObjInp)
{
#ifdef RODS_CAT
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput;
    int status;

    condInput = unregDataObjInp->condInput;
    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = chlUnregDataObj (rsComm, dataObjInfo, condInput);
    return (status);
#else
    return (SYS_NO_RCAT_SERVER_ERR);
#endif

}

