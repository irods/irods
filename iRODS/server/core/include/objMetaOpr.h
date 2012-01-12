/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* objMetaOpr.h - header file for objMetaOpr.c
 */



#ifndef OBJ_META_OPR_H
#define OBJ_META_OPR_H

#include "rods.h"
#include "initServer.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#if 0
#include "ruleExecSubmit.h"
#include "reIn2p3SysRule.h"
#endif
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"

#ifdef  __cplusplus
extern "C" {
#endif

int
svrCloseQueryOut (rsComm_t *rsComm, genQueryOut_t *genQueryOut);
int
isData(rsComm_t *rsComm, char *objName, rodsLong_t *dataId);
int
isColl(rsComm_t *rsComm, char *objName, rodsLong_t *collId);
int
isCollAllKinds (rsComm_t *rsComm, char *objName, rodsLong_t *collId);
int
isUser(rsComm_t *rsComm, char *objName);
int
isResc(rsComm_t *rsComm, char *objName);
int
isRescGroup(rsComm_t *rsComm, char *objName);
int
isMeta(rsComm_t *rsComm, char *objName);
int
isToken(rsComm_t *rsComm, char *objName);
int
getObjType(rsComm_t *rsComm, char *objName, char * objType);
int
addAVUMetadataFromKVPairs (rsComm_t *rsComm, char *objName, char *inObjType,
                           keyValPair_t *kVP);
int
removeAVUMetadataFromKVPairs(rsComm_t *rsComm, char *objName, char *inObjType,
                           keyValPair_t *kVP);
int
getStructFileType (specColl_t *specColl);

extern int
checkPermissionByObjType(rsComm_t *rsComm, char *objName, char *objType, char *user, char *zone, char *oper);
#ifdef  __cplusplus
}
#endif

#endif	/* OBJ_META_OPR_H */
