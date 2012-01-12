/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* auditUtil.h - header file for auditUtil.c
 */

#ifndef auditUtil_H
#define auditUtil_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


int getAuditTrailInfoByUserID(char *userID, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getAuditTrailInfoByObjectID(char *objectID, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getAuditTrailInfoByActionID(char *actionID, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getAuditTrailInfoByKeywords(char *commentStr, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getAuditTrailInfoByTimeStamp(char *begTS, char *endTS, bytesBuf_t *mybuf, rsComm_t *rsComm);


#endif	/* auditUtil_H */

