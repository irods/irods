/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* dataObjOpr.h - header file for dataObjOpr.c
 */



#ifndef DATA_OBJ_OPR_H
#define DATA_OBJ_OPR_H

#include "rods.h"
#include "initServer.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "ruleExecSubmit.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"
#include "reIn2p3SysRule.h"

/* definition for return value of resolveSingleReplCopy */
#define NO_GOOD_COPY	0
#define HAVE_GOOD_COPY	1

/* definition for trimjFlag in matchAndTrimRescGrp */
#define TRIM_MATCHED_RESC_INFO		0x1
#define REQUE_MATCHED_RESC_INFO		0x2
#define TRIM_MATCHED_OBJ_INFO		0x4
#define TRIM_UNMATCHED_OBJ_INFO		0x8

int
getDataObjInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead, char *accessPerm, int ignoreCondInput);
int
updateDataObjReplStatus (rsComm_t *rsComm, int l1descInx, int replStatus);
int
dataObjExist (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
int
sortObjInfoForRepl (dataObjInfo_t **dataObjInfoHead, 
dataObjInfo_t **oldDataObjInfoHead, int deleteOldFlag);
int
sortObjInfoForOpen (rsComm_t *rsComm, dataObjInfo_t **dataObjInfoHead, 
keyValPair_t *condInput, int writeFlag);
int
sortDataObjInfoRandom (dataObjInfo_t **dataObjInfoHead);
int
requeDataObjInfoByResc (dataObjInfo_t **dataObjInfoHead, char *preferedResc,
int writeFlag, int topFlag);
int
requeDataObjInfoByReplNum (dataObjInfo_t **dataObjInfoHead, int replNum);
dataObjInfo_t *
chkCopyInResc (dataObjInfo_t *dataObjInfoHead, rescGrpInfo_t *myRescGrpInfo);
int
chkAndTrimCopyInRescGrp (dataObjInfo_t **dataObjInfoHead, 
rescGrpInfo_t **rescGrpInfoHead, int trimDataObjFlag);
int
initDataObjInfoQuery (dataObjInp_t *dataObjInp, genQueryInp_t *genQueryInp,
int ignoreCondInput);
int
sortObjInfo (dataObjInfo_t **dataObjInfoHead,
dataObjInfo_t **dirtyArchInfo, dataObjInfo_t **dirtyCacheInfo,
dataObjInfo_t **oldArchInfo, dataObjInfo_t **oldCacheInfo,
dataObjInfo_t **downCurrentInfo, dataObjInfo_t **downOldInfo);
int
chkOrphanFile (rsComm_t *rsComm, char *filePath, char *rescName,
dataObjInfo_t *dataObjInfo);
int
chkOrphanDir (rsComm_t *rsComm, char *dirPath, char *rescName);
int
getNumDataObjInfo (dataObjInfo_t *dataObjInfoHead);
int
resolveSingleReplCopy (dataObjInfo_t **dataObjInfoHead,
dataObjInfo_t **oldDataObjInfoHead, rescGrpInfo_t **destRescGrpInfo,
dataObjInfo_t **destDataObjInfo, keyValPair_t *condInput);
int
matchDataObjInfoByCondInput (dataObjInfo_t **dataObjInfoHead,
dataObjInfo_t **oldDataObjInfoHead, keyValPair_t *condInput,
dataObjInfo_t **matchedDataObjInfo, dataObjInfo_t **matchedOldDataObjInfo);
int
resolveInfoForPhymv (dataObjInfo_t **dataObjInfoHead,
dataObjInfo_t **oldDataObjInfoHead, rescGrpInfo_t **destRescGrpInfo,
keyValPair_t *condInput, int multiCopyFlag);
int
matchAndTrimRescGrp (dataObjInfo_t **dataObjInfoHead,
rescGrpInfo_t **rescGrpInfoHead, int trimjFlag,
dataObjInfo_t **trimmedDataObjInfo);
int
resolveInfoForTrim (dataObjInfo_t **dataObjInfoHead,
keyValPair_t *condInput);
int
requeDataObjInfoByDestResc (dataObjInfo_t **dataObjInfoHead,
keyValPair_t *condInput, int writeFlag, int topFlag);
int
requeDataObjInfoBySrcResc (dataObjInfo_t **dataObjInfoHead,
keyValPair_t *condInput, int writeFlag, int topFlag);
int
getDataObjInfoIncSpecColl (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfo);
int
regNewObjSize (rsComm_t *rsComm, char *objPath, int replNum,
rodsLong_t newSize);
int
getCacheDataInfoForRepl (rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, dataObjInfo_t *compDataObjInfo,
dataObjInfo_t **outDataObjInfo);
int
getNonGrpCacheDataInfoInRescGrp (dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, rescGrpInfo_t *rescGrpInfo,
dataObjInfo_t *compDataObjInfo, dataObjInfo_t **outDataObjInfo);
int
getCacheDataInfoInRescGrp (dataObjInfo_t *srcDataObjInfoHead,
dataObjInfo_t *destDataObjInfoHead, char *rescGroupName,
dataObjInfo_t *compDataObjInfo, dataObjInfo_t **outDataObjInfo);
#endif	/* DATA_OBJ_OPR_H */
