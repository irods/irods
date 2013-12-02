/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reDataObjOpr.h - header file for reDataObjOpr.c
 */



#ifndef RE_DATA_OBJ_OPR_H
#define RE_DATA_OBJ_OPR_H

#include "rods.h"
#include "objMetaOpr.h"
#include "dataObjRepl.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

/* the following are data object operation rule handler */

int
msiDataObjCreate (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjOpen (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiDataObjClose (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiDataObjLseek (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiDataObjRead (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjWrite (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjUnlink (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiDataObjRepl (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjCopy (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjPut (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjGet (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjChksum (msParam_t *inpParam1, msParam_t *inpParam2, 
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjPhymv (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjRename (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjTrim (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *inpParam5,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiCollCreate (msParam_t *inpParam1, msParam_t *inpParam2, 
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiRmColl (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiReplColl (msParam_t *coll, msParam_t *destRescName, msParam_t *options,
  msParam_t *outParam, ruleExecInfo_t *rei);
int
msiCollRepl (msParam_t *collection, msParam_t *targetResc, msParam_t *status, 
  ruleExecInfo_t *rei);
int
msiPhyPathReg (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiObjStat (msParam_t *inpParam1, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjRsync (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiExecCmd (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
msParam_t *inpParam4, msParam_t *inpParam5, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiDataObjReplWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjChksumWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiDataObjGetWithOptions (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *srcrescParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiTarFileExtract (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3,  msParam_t *outParam, ruleExecInfo_t *rei);
int
msiTarFileCreate (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3,  msParam_t *outParam, ruleExecInfo_t *rei);
int
msiPhyBundleColl (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiCollRsync (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *inpParam4, msParam_t *outParam,
ruleExecInfo_t *rei);
int
_rsCollRsync (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
char *srcColl, char *destColl);
#endif	/* RE_DATA_OBJ_OPR_H */
