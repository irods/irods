/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* ftpMS.h - header file for ftpMS.c
 */

#ifndef FTPMS_H
#define FTPMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


typedef struct {
	char objPath[MAX_NAME_LEN];
	int l1descInx;
	rsComm_t *rsComm;
} dataObjFtpInp_t;


int msiFtpGet(msParam_t *target, msParam_t *destObj, msParam_t *status, ruleExecInfo_t *rei);
int msiTwitterPost(msParam_t *twittername, msParam_t *twitterpass, msParam_t *message, msParam_t *status, ruleExecInfo_t *rei);
int msiPostThis(msParam_t *url, msParam_t *data, msParam_t *status, ruleExecInfo_t *rei);


#endif	/* FTPMS_H */

