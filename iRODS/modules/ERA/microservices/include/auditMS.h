/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* auditMS.h - header file for auditMS.c
 */

#ifndef AUDITMS_H
#define AUDITMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


int msiGetAuditTrailInfoByUserID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetAuditTrailInfoByObjectID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetAuditTrailInfoByActionID(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetAuditTrailInfoByKeywords(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam, ruleExecInfo_t *rei);
int msiGetAuditTrailInfoByTimeStamp(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);


#endif	/* AUDITMS_H */


