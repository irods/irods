/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* dataObjOpr.h - header file for dataObjOpr.c
 */



#ifndef DATA_OBJ_OPR_HPP
#define DATA_OBJ_OPR_HPP

#include "rods.hpp"
#include "objInfo.hpp"
#include "dataObjInpOut.hpp"
#include "ruleExecSubmit.hpp"
#include "rcGlobalExtern.hpp"
#include "rsGlobalExtern.hpp"
#include "reIn2p3SysRule.hpp"

/* definition for return value of resolveSingleReplCopy */
#define NO_GOOD_COPY    0
#define HAVE_GOOD_COPY  1

/* definition for trimjFlag in matchAndTrimRescGrp */
#define TRIM_MATCHED_RESC_INFO          0x1
#define REQUE_MATCHED_RESC_INFO         0x2
#define TRIM_MATCHED_OBJ_INFO           0x4
#define TRIM_UNMATCHED_OBJ_INFO         0x8

int
getDataObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                dataObjInfo_t **dataObjInfoHead, char *accessPerm, int ignoreCondInput );
int
updateDataObjReplStatus( rsComm_t *rsComm, int l1descInx, int replStatus );
int
dataObjExist( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
sortObjInfoForRepl( dataObjInfo_t **dataObjInfoHead,
                    dataObjInfo_t **oldDataObjInfoHead,
                    int deleteOldFlag, const char* resc_hier,
                    const char* dst_resc_hier );
int
sortObjInfoForOpen( dataObjInfo_t **dataObjInfoHead,
                    keyValPair_t *condInput, int writeFlag );
int
sortDataObjInfoRandom( dataObjInfo_t **dataObjInfoHead );
int
requeDataObjInfoByResc( dataObjInfo_t **dataObjInfoHead, const char *preferedResc,
                        int writeFlag, int topFlag );
int
requeDataObjInfoByReplNum( dataObjInfo_t **dataObjInfoHead, int replNum );
dataObjInfo_t *
chkCopyInResc( dataObjInfo_t *&dataObjInfoHead, const std::string& _resc_name, const char* destRescHier );

int
initDataObjInfoQuery( dataObjInp_t *dataObjInp, genQueryInp_t *genQueryInp,
                      int ignoreCondInput );
int
sortObjInfo( dataObjInfo_t **dataObjInfoHead,
             dataObjInfo_t **dirtyArchInfo, dataObjInfo_t **dirtyCacheInfo,
             dataObjInfo_t **oldArchInfo, dataObjInfo_t **oldCacheInfo,
             dataObjInfo_t **downCurrentInfo, dataObjInfo_t **downOldInfo );
int
chkOrphanFile( rsComm_t *rsComm, char *filePath, const char *rescName,
               dataObjInfo_t *dataObjInfo );
int
chkOrphanDir( rsComm_t *rsComm, char *dirPath, const char *rescName );
int
getNumDataObjInfo( dataObjInfo_t *dataObjInfoHead );
int
resolveSingleReplCopy( dataObjInfo_t **dataObjInfoHead,
                       dataObjInfo_t **oldDataObjInfoHead,
                       const std::string& _resc_name,
                       dataObjInfo_t **destDataObjInfo, keyValPair_t *condInput );
int
matchDataObjInfoByCondInput( dataObjInfo_t **dataObjInfoHead,
                             dataObjInfo_t **oldDataObjInfoHead, keyValPair_t *condInput,
                             dataObjInfo_t **matchedDataObjInfo, dataObjInfo_t **matchedOldDataObjInfo );
int
resolveInfoForPhymv( dataObjInfo_t **dataObjInfoHead,
                     dataObjInfo_t **oldDataObjInfoHead, const std::string& _resc_name,
                     keyValPair_t *condInput, int multiCopyFlag );
int
matchAndTrimRescGrp( dataObjInfo_t **dataObjInfoHead,
                     const std::string& _resc_name,
                     int trimjFlag,
                     dataObjInfo_t **trimmedDataObjInfo );
int
resolveInfoForTrim( dataObjInfo_t **dataObjInfoHead,
                    keyValPair_t *condInput );
int
requeDataObjInfoByDestResc( dataObjInfo_t **dataObjInfoHead,
                            keyValPair_t *condInput, int writeFlag, int topFlag );
int
requeDataObjInfoBySrcResc( dataObjInfo_t **dataObjInfoHead,
                           keyValPair_t *condInput, int writeFlag, int topFlag );
int
getDataObjInfoIncSpecColl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           dataObjInfo_t **dataObjInfo );
int
regNewObjSize( rsComm_t *rsComm, char *objPath, int replNum,
               rodsLong_t newSize );
int
getDataObjByClass( dataObjInfo_t *dataObjInfoHead, int rescClass,
                   dataObjInfo_t **outDataObjInfo );
#endif  /* DATA_OBJ_OPR_H */
