/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reSysDataObjOpr.h - header file for reSysDataObjOpr.c
 */



#ifndef RE_NETCDF_H
#define RE_NETCDF_H

#include "apiHeaderAll.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"

int
msiNcOpen (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcCreate (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcClose (msParam_t *inpParam1, ruleExecInfo_t *rei);
int
msiNcInqId (msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcInqWithId (msParam_t *inpParam1, msParam_t *inpParam2,
msParam_t *inpParam3, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetVarsByType (msParam_t *dataTypeParam, msParam_t *ncidParam,
msParam_t *varidParam, msParam_t *ndimParam, msParam_t *startParam,
msParam_t *countParam, msParam_t *strideParam,
msParam_t *outParam, ruleExecInfo_t *rei);
#ifdef LIB_CF
int
msiNccfGetVara (msParam_t *ncidParam, msParam_t *varidParam,
msParam_t *lvlIndexParam, msParam_t *timestepParam,
msParam_t *latRange0Param, msParam_t *latRange1Param,
msParam_t *lonRange0Param, msParam_t *lonRange1Param,
msParam_t *maxOutArrayLenParam, msParam_t *outParam, ruleExecInfo_t *rei);
#endif
int
msiNcGetArrayLen (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetNumDim (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetDataType (msParam_t *inpParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetElementInArray (msParam_t *arrayStructParam, msParam_t *indexParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiFloatToString (msParam_t *floatParam, msParam_t *stringParam,
ruleExecInfo_t *rei);
int
msiNcInq (msParam_t *ncidParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetNdimsInInqOut (msParam_t *ncInqOutParam, msParam_t *varNameParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetNattsInInqOut (msParam_t *ncInqOutParam, msParam_t *varNameParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetNvarsInInqOut (msParam_t *ncInqOutParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetFormatInInqOut (msParam_t *ncInqOutParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetVarNameInInqOut (msParam_t *ncInqOutParam, msParam_t *inxParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetDimNameInInqOut (msParam_t *ncInqOutParam, msParam_t *inxParam,
msParam_t *varNameParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetAttNameInInqOut (msParam_t *ncInqOutParam, msParam_t *inxParam,
msParam_t *varNameParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetVarTypeInInqOut (msParam_t *ncInqOutParam, msParam_t *varNameParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcIntDataTypeToStr (msParam_t *dataTypeParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetAttValStrInInqOut (msParam_t *ncInqOutParam, msParam_t *whichAttParam,
msParam_t *varNameParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
_msiNcGetAttValInInqOut (msParam_t *ncInqOutParam, msParam_t *whichAttParam,
msParam_t *varNameParam, ncGetVarOut_t **ncGetVarOut);
int
msiAddToNcArray (msParam_t *elementParam, msParam_t *inxParam,
msParam_t *ncArrayParam, ruleExecInfo_t *rei);
int
msiNcGetDimLenInInqOut (msParam_t *ncInqOutParam, msParam_t *inxParam,
msParam_t *varNameParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetVarIdInInqOut (msParam_t *ncInqOutParam, msParam_t *whichVarParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiFreeNcStruct (msParam_t *inpParam, ruleExecInfo_t *rei);
int
msiNcOpenGroup (msParam_t *rootNcidParam, msParam_t *fullGrpNameParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcInqGrps (msParam_t *ncidParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcGetNGrpsInInqOut (msParam_t *ncInqGrpsOutParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiNcGetGrpInInqOut (msParam_t *ncInqGrpsOutParam,
msParam_t *inxParam, msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcRegGlobalAttr (msParam_t *objPathParam, msParam_t *adminParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
msiNcSubsetVar (msParam_t *varNameParam, msParam_t *ncidParam,
msParam_t *ncInqOutParam, msParam_t *subsetStrParam,
msParam_t *outParam, ruleExecInfo_t *rei);
int
ncSubsetVar (rsComm_t *rsComm, int ncid, ncInqOut_t *ncInqOut,
ncVarSubset_t *ncVarSubset, ncGetVarOut_t **ncGetVarOut);
int
msiNcVarStat (msParam_t *ncGetVarOutParam, msParam_t *statOutStr,
ruleExecInfo_t *rei);
int
procMaxMinAve (float myfloat, float *mymax, float *mymin, float *mytotal);

#endif	/* RE_NETCDF_H */
