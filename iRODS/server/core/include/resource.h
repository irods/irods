/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* resource.h - header file for resource.c
 */



#ifndef RESOURCE_H
#define RESOURCE_H

#include "rods.h"
#include "initServer.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "ruleExecSubmit.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"
#include "reIn2p3SysRule.h"
#include "reSysDataObjOpr.h"

/* definition for the flag in queRescGrp and queResc */
#define BOTTOM_FLAG     0
#define TOP_FLAG        1
#define BY_TYPE_FLAG    2

#define MAX_ELAPSE_TIME 1800 /* max time in seconds above which the load 
			      * info is considered to be out of date */

#ifdef  __cplusplus
extern "C" {
#endif


int
getRescInfo (rsComm_t *rsComm, char *defaultResc, keyValPair_t *condInput,
rescGrpInfo_t **rescGrpInfo);
int
getRescStatus (rsComm_t *rsComm, char *inpRescName, keyValPair_t *condInput);
int
_getRescInfo (rsComm_t *rsComm, char *rescName, rescGrpInfo_t **rescGrpInfo);
int
resolveAndQueResc (char *rescName, char *rescGrpName,
rescGrpInfo_t **rescGrpInfo);
int
resolveRescGrp (rsComm_t *rsComm, char *rescGroupName,
rescGrpInfo_t **rescGrpInfo);
int
resolveResc (char *rescName, rescInfo_t **rescInfo);
int
getNumResc (rescGrpInfo_t *rescGrpInfo);
int
sortResc (rsComm_t *rsComm, rescGrpInfo_t **rescGrpInfo,
char *sortScheme);
int
getRescClass (rescInfo_t *rescInfo);
int
getRescGrpClass (rescGrpInfo_t *rescGrpInfo, rescInfo_t **outRescInfo);
int
compareRescAddr (rescInfo_t *srcRescInfo, rescInfo_t *destRescInfo);
int
getCacheRescInGrp (rsComm_t *rsComm, char *rescGroupName,
rescInfo_t *memberRescInfo, rescInfo_t **outCacheResc);
int
getRescInGrp (rsComm_t *rsComm, char *rescName, char *rescGroupName,
rescInfo_t **outRescInfo);
int
sortRescByType (rescGrpInfo_t **rescGrpInfo);
int
sortRescByLocation (rescGrpInfo_t **rescGrpInfo);
int
sortRescByLoad (rsComm_t *rsComm, rescGrpInfo_t **rescGrpInfo);
int
initRescGrp (rsComm_t *rsComm);
int
getRescGrpOfResc (rsComm_t *rsComm, rescInfo_t * rescInfo,
rescGrpInfo_t **rescGrpInfo);
int
isRescsInSameGrp (rsComm_t *rsComm, char *rescName1, char *rescName2,
rescGrpInfo_t **outRescGrpInfo);
int
replRescGrpInfo (rescGrpInfo_t *srcRescGrpInfo,
rescGrpInfo_t **destRescGrpInfo);
int
sortRescRandom (rescGrpInfo_t **rescGrpInfo);
int
setDefaultResc (rsComm_t *rsComm, char *defaultRescList, char *optionStr,
keyValPair_t *condInput, rescGrpInfo_t **outRescGrpInfo);
int
getRescInfoAndStatus (rsComm_t *rsComm, char *rescName, keyValPair_t *condInput,
rescGrpInfo_t **rescGrpInfo);
int
queryRescInRescGrp (rsComm_t *rsComm, char *rescGroupName,
genQueryOut_t **genQueryOut);
int
initResc (rsComm_t *rsComm);
int
getHostStatusByRescInfo (rodsServerHost_t *rodsServerHost);
int
procAndQueRescResult (genQueryOut_t *genQueryOut);
int
printLocalResc ();
int
queResc (rescInfo_t *myRescInfo, char *rescGroupName,
rescGrpInfo_t **rescGrpInfoHead, int topFlag);
int
queRescGrp (rescGrpInfo_t **rescGrpInfoHead, rescGrpInfo_t *myRescGrpInfo,
int flag);
int
freeAllRescGrp (rescGrpInfo_t *rescGrpHead);
int
getRescTypeInx (char *rescType);
int
getRescType (rescInfo_t *rescInfo);
int
getRescClassInx (char *rescClass);
int
getMultiCopyPerResc ();
int
getRescCnt (rescGrpInfo_t *myRescGrpInfo);
int
updateResc (rsComm_t *rsComm);
rescInfo_t *
matchSameHostRescByType (rescInfo_t *myRescInfo, int driverType);
#ifdef  __cplusplus
}
#endif

#endif	/* RESOURCE_H */
