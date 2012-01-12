/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "modDataObjMeta.h"
#include "icatHighLevelRoutines.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"

int
rsModDataObjMeta (rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;

    regParam = modDataObjMetaInp->regParam;
    dataObjInfo = modDataObjMetaInp->dataObjInfo;

    status = getAndConnRcatHost (rsComm, MASTER_RCAT, dataObjInfo->objPath,
      &rodsServerHost);
    if (status < 0) {
       return(status);
    }
    if (rodsServerHost->localFlag == LOCAL_HOST) {
#ifdef RODS_CAT
        status = _rsModDataObjMeta (rsComm, modDataObjMetaInp);
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    } else {
        status = rcModDataObjMeta (rodsServerHost->conn, modDataObjMetaInp);
    }

    return (status);
}

int
_rsModDataObjMeta (rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp)
{
#ifdef RODS_CAT
    int status;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;
    int i;
    ruleExecInfo_t rei2;

    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    rei2.rsComm = rsComm;
    if (rsComm != NULL) {
      rei2.uoic = &rsComm->clientUser;
      rei2.uoip = &rsComm->proxyUser;
    }
    rei2.doi = modDataObjMetaInp->dataObjInfo;
    rei2.condInputData = modDataObjMetaInp->regParam;
    regParam = modDataObjMetaInp->regParam;
    dataObjInfo = modDataObjMetaInp->dataObjInfo;

    if (regParam->len == 0) {
        return (0);
    }

    /* In dataObjInfo, need just dataId. But it will accept objPath too,
     * but less efficient 
     */

    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    rei2.doi = dataObjInfo;
    i =  applyRule("acPreProcForModifyDataObjMeta",NULL, &rei2, NO_SAVE_REI);
    if (i < 0) {
      if (rei2.status < 0) {
        i = rei2.status;
      }
      rodsLog (LOG_ERROR,
               "_rsModDataObjMeta:acPreProcForModifyDataObjMeta error stat=%d", i);
      return i;
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    if (getValByKey (regParam, ALL_KW) != NULL) {
	/* all copies */
	dataObjInfo_t *dataObjInfoHead = NULL;
	dataObjInfo_t *tmpDataObjInfo;
	dataObjInp_t dataObjInp;

	bzero (&dataObjInp, sizeof (dataObjInp));
	rstrcpy (dataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN);
        status = getDataObjInfoIncSpecColl (rsComm, &dataObjInp,
          &dataObjInfoHead);
	if (status < 0) return status;
	tmpDataObjInfo = dataObjInfoHead;
        while (tmpDataObjInfo != NULL) {
	    if (tmpDataObjInfo->specColl != NULL) break;
            status = chlModDataObjMeta (rsComm, tmpDataObjInfo, regParam);
	    if (status < 0) {
                rodsLog (LOG_ERROR,
                  "_rsModDataObjMeta:chlModDataObjMeta %s error stat=%d",
	          tmpDataObjInfo->objPath, status);
	    }
	    tmpDataObjInfo = tmpDataObjInfo->next;
	}
	freeAllDataObjInfo (dataObjInfoHead);
    } else {
        status = chlModDataObjMeta (rsComm, dataObjInfo, regParam);
    }

    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/
    if (status >= 0) {
      i =  applyRule("acPostProcForModifyDataObjMeta",NULL, &rei2, NO_SAVE_REI);
      if (i < 0) {
        if (rei2.status < 0) {
          i = rei2.status;
        }
        rodsLog (LOG_ERROR,
           "_rsModDataObjMeta:acPostProcForModifyDataObjMeta error stat=%d",i);
        return i;
      }
    }
    /** RAJA ADDED June 1 2009 for pre-post processing rule hooks **/

    return (status);
#else
    return (SYS_NO_RCAT_SERVER_ERR);
#endif

}


