/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* eraMS.h - header file for eraMS.c
 */

#ifndef ERAMS_H
#define ERAMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"
#include "miscUtil.h"



int msiRecursiveCollCopy(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiCopyAVUMetadata(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetDataObjAVUs(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetDataObjPSmeta(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetCollectionPSmeta(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiLoadMetadataFromDataObj(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetDataObjAIP(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiExportRecursiveCollMeta(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetDataObjACL(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetCollectionACL(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetUserInfo(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetUserACL(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiCreateUserAccountsFromDataObj(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiLoadUserModsFromDataObj(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiDeleteUsersFromDataObj(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiLoadACLFromDataObj(msParam_t *inpParam, msParam_t *outParam, ruleExecInfo_t *rei);
int msiSetDataType(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGuessDataType(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiMergeDataCopies(msParam_t *objPath, msParam_t *currentColl, msParam_t *masterColl, msParam_t *status, ruleExecInfo_t *rei);
int msiFlagDataObjwithAVU(msParam_t *dataObj, msParam_t *flag, msParam_t *status, ruleExecInfo_t *rei);
int msiFlagInfectedObjs(msParam_t *scanResObj, msParam_t *scanResc, msParam_t *status, ruleExecInfo_t *rei);
int msiStripAVUs(msParam_t *target, msParam_t *options, msParam_t *status, ruleExecInfo_t *rei);
int msiStoreVersionWithTS(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);

#endif	/* ERAMS_H */

