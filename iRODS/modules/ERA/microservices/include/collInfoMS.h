/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* collInfoMS.h - header file for collInfoMS.c
 */

#ifndef COLLINFOMS_H
#define COLLINFOMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


int msiIsColl(msParam_t *targetPath, msParam_t *collId, msParam_t *status, ruleExecInfo_t *rei);
int msiIsData(msParam_t *targetPath, msParam_t *dataId, msParam_t *status, ruleExecInfo_t *rei);
int msiGetObjectPath(msParam_t *object, msParam_t *path, msParam_t *status, ruleExecInfo_t *rei);
int msiGetCollectionContentsReport(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetCollectionSize(msParam_t *collPath, msParam_t *outKVPairs, msParam_t *status, ruleExecInfo_t *rei);
int msiStructFileBundle(msParam_t *collection, msParam_t *bundleObj, msParam_t *resource, msParam_t *status, ruleExecInfo_t *rei);
int msiCollectionSpider(msParam_t *collection, msParam_t *objects, msParam_t *action, msParam_t *status, ruleExecInfo_t *rei);


#endif	/* COLLINFOMS_H */

