#ifndef IRODS_RE_SYS_DATA_OBJ_OPR_HPP
#define IRODS_RE_SYS_DATA_OBJ_OPR_HPP

#include "irods/rods.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjRepl.h"
#include "irods/modDataObjMeta.h"

int
msiSetDataObjPreferredResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
int
msiSetDataObjAvoidResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
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
msiSetRescQuotaPolicy( msParam_t *xflag, ruleExecInfo_t *rei );

#endif // IRODS_RE_SYS_DATA_OBJ_OPR_HPP
