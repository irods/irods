/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reSysDataObjOpr.h - header file for reSysDataObjOpr.c
 */



#ifndef RE_SYS_DATA_OBJ_OPR_HPP
#define RE_SYS_DATA_OBJ_OPR_HPP

#include "rods.h"
#include "objMetaOpr.hpp"
#include "dataObjRepl.h"
#include "modDataObjMeta.h"


/* the following are data object operation rule handler */
int
msiSysReplDataObj( msParam_t *xcacheResc, msParam_t *xallFlag,
                   ruleExecInfo_t *rei );
int
msiStageDataObj( msParam_t *cacheResc, ruleExecInfo_t *rei );
int
msiSetDataObjPreferredResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
int
msiSetDataObjAvoidResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
int
msiSortDataObj( msParam_t *sortScheme, ruleExecInfo_t *rei );
int
msiSysChksumDataObj( ruleExecInfo_t *rei );
int
msiSetDataTypeFromExt( ruleExecInfo_t *rei );
int
msiSetNoDirectRescInp( msParam_t *rescList, ruleExecInfo_t *rei );
int
msiSetDefaultResc( msParam_t *defaultResc, msParam_t *forceStr, ruleExecInfo_t *rei );
int
msiSetNumThreads( msParam_t *sizePerThrInMbStr, msParam_t *maxNumThrStr,
                  msParam_t *windowSizeStr, ruleExecInfo_t *rei );
int
msiDeleteDisallowed( ruleExecInfo_t *rei );
int
msiOprDisallowed( ruleExecInfo_t *rei );
int
msiNoChkFilePathPerm( ruleExecInfo_t *rei );
int // JMC - backport 4774
msiSetChkFilePathPerm( msParam_t *xchkType, ruleExecInfo_t *rei );
int
msiNoTrashCan( ruleExecInfo_t *rei );
int
msiSetPublicUserOpr( msParam_t *xoprList, ruleExecInfo_t *rei );
int
setApiPerm( int apiNumber, int proxyPerm, int clientPerm );
int
msiSetGraftPathScheme( msParam_t *xaddUserName, msParam_t *xtrimDirCnt,
                       ruleExecInfo_t *rei );
int
msiSetRandomScheme( ruleExecInfo_t *rei );
int
msiSetReServerNumProc( msParam_t *xnumProc, ruleExecInfo_t *rei );
int
msiSetRescQuotaPolicy( msParam_t *xflag, ruleExecInfo_t *rei );
int
msiSysMetaModify( msParam_t *sysMetadata, msParam_t *value, ruleExecInfo_t *rei );
#endif	/* RE_SYS_DATA_OBJ_OPR_H */
