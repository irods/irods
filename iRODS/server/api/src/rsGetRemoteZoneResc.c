/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGetRemoteZoneResc.c
 */

#include "getRemoteZoneResc.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "regDataObj.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "dataObjCreate.h"
#include "dataObjOpen.h"

int
rsGetRemoteZoneResc (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsHostAddr_t **rescAddr)
{
    char *remoteOprType;
    int status;
    rescInfo_t *rescInfo = NULL;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    dataObjInfo_t *dataObjInfoHead = NULL;

    *rescAddr = NULL;
    if ((remoteOprType = 
      getValByKey (&dataObjInp->condInput, REMOTE_ZONE_OPR_KW)) == NULL) {
        rodsLog (LOG_ERROR,
          "rsGetRemoteZoneResc: EMOTE_ZONE_OPR_KW not defined for %s",
          dataObjInp->objPath);
	return (USER_BAD_KEYWORD_ERR);
    }

    if (strcmp (remoteOprType, REMOTE_CREATE) == 0) {
	status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
        if (status < 0) return status;
	    rescInfo = myRescGrpInfo->rescInfo;
    } else if (strcmp (remoteOprType, REMOTE_OPEN) == 0) {
        status = getDataObjInfoIncSpecColl (rsComm, dataObjInp, 
	  &dataObjInfoHead);
	if (status < 0) {
	    /* does not exist */
	    status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
            if (status < 0) return status;
	    rescInfo = myRescGrpInfo->rescInfo;
	} else {
	    int writeFlag;
	    /* exist */
	    writeFlag = getWriteFlag (dataObjInp->openFlags);
	    status = sortObjInfoForOpen (rsComm, &dataObjInfoHead, 
	      &dataObjInp->condInput, writeFlag);
	    if (status < 0) return status;
            status = applyPreprocRuleForOpen (rsComm, dataObjInp, 
	     &dataObjInfoHead);
	    if (status < 0) {
		freeAllDataObjInfo (dataObjInfoHead);
		return status;
	    } else {	
	        rescInfo = dataObjInfoHead->rescInfo;
		freeAllDataObjInfo (dataObjInfoHead);
	    }
	}
    } else {
	rodsLog (LOG_ERROR,
          "rsGetRemoteZoneResc: bad EMOTE_ZONE_OPR_KW %s for %s",
          remoteOprType, dataObjInp->objPath);
        return (USER_BAD_KEYWORD_ERR);
    }
    *rescAddr = (rodsHostAddr_t*)malloc (sizeof (rodsHostAddr_t));
    bzero (*rescAddr, sizeof (rodsHostAddr_t));
    rstrcpy ((*rescAddr)->hostAddr, rescInfo->rescLoc, NAME_LEN);
    rstrcpy ((*rescAddr)->zoneName, ZoneInfoHead->zoneName, NAME_LEN);
    if (myRescGrpInfo != NULL) freeAllRescGrpInfo (myRescGrpInfo);

    return (0);
}

