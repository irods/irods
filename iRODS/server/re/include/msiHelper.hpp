/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* msiHelper.h - header file for msiHelper.c
 */



#ifndef MSI_HELPER_H
#define MSI_HELPER_H

#include "rods.h"
#include "objMetaOpr.h"
#include "dataObjRepl.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"


int
msiGetStdoutInExecCmdOut (msParam_t *inpExecCmdOut, msParam_t *outStr,
ruleExecInfo_t *rei);
int
msiGetStderrInExecCmdOut (msParam_t *inpExecCmdOut, msParam_t *outStr,
ruleExecInfo_t *rei);
int
msiAddKeyValToMspStr (msParam_t *keyStr, msParam_t *valStr,
msParam_t *msKeyValStr, ruleExecInfo_t *rei);
int
msiWriteRodsLog (msParam_t *inpParam1,  msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiSplitPath (msParam_t *inpPath,  msParam_t *outParentColl, 
msParam_t *outChildName, ruleExecInfo_t *rei);
int
msiSplitPathByKey (msParam_t *inpPath,  msParam_t *inpKey, msParam_t *outParentColl,
msParam_t *outChildName, ruleExecInfo_t *rei);
int
msiGetSessionVarValue (msParam_t *inpVar,  msParam_t *outputMode,
ruleExecInfo_t *rei);
int
msiStrlen (msParam_t *stringIn,  msParam_t *lengthOut, ruleExecInfo_t *rei);
int
msiStrchop (msParam_t *stringIn,  msParam_t *stringOut, ruleExecInfo_t *rei);
int
msiSubstr (msParam_t *stringIn,  msParam_t *offset, msParam_t *length,
msParam_t *stringOut, ruleExecInfo_t *rei);
int
msiExit (msParam_t *inpParam1, msParam_t *inpParam2, ruleExecInfo_t *rei);
int
msiStrCat (msParam_t *targParam, msParam_t *srcParam, ruleExecInfo_t *rei);
#endif  /* MSI_HELPER_H */


