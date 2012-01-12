/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef RE_FUNCDEFS_H
#define RE_FUNCDEFS_H

/* definition for freeSpeialStructFlag in freeRuleExecInfoStruct and
 * freeRuleExecInfoInternals
 */
#define FREE_MS_PARAM	0x1
#define FREE_DOINP	0x2

#ifdef RULE_ENGINE_N
#include "restructs.h"
int initRuleStruct(int processType, rsComm_t *svrComm, char *ruleSet, char *dvmSet, char *fnmSet);

int readRuleStructFromFile(int processType, char *ruleBaseName, ruleStruct_t *inRuleStrct);
int computeExpression( char *expr, msParamArray_t *msParamArray, ruleExecInfo_t *rei, int reiSaveFlag, char *res);

#else
int initRuleStruct(rsComm_t *svrComm, char *ruleSet, char *dvmSet, char *fnmSet);
int readRuleStructFromFile(char *ruleBaseName, ruleStruct_t *inRuleStrct);
int computeExpression( char *expr, ruleExecInfo_t *rei, int reiSaveFlag, char *res);

#endif

int clearRuleStruct(ruleStruct_t *inRuleStrct);

int readDVarStructFromFile(char *dvarBaseName,rulevardef_t *inRuleVarDef);

int clearDVarStruct(rulevardef_t *inRuleVarDef);

int readFuncMapStructFromFile(char *fmapBaseName, rulefmapdef_t* inRuleFuncMapDef);

int clearFuncMapStruct( rulefmapdef_t* inRuleFuncMapDef);

int findNextRule (char *action,  int *ruleInx);

int getRule(int ri, char *ruleBase, char *ruleHead, char *ruleCondition, 
	char *ruleAction, char *ruleRecovery, int rSize);

int executeRuleBody(char *action, char *ruleAction, char *ruleRecovery, 
		char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		ruleExecInfo_t *rei, int reiSaveFlag);

int getActionRecoveryList(char *ruleAction, char *ruleRecovery, 
		      char *actionArray[],char *recoveryArray[]);

int executeRuleRecovery(char *ruleRecovery, ruleExecInfo_t *rei, int reiSaveFlag);

int parseAction(char *inAction,char *action, char *args[], int *argc);

int executeRuleAction(char *inAction, ruleExecInfo_t *rei, int reiSaveFlag);

int actionTableLookUp (char *action);

int getNextAction(char *listOfAction, char *action, char **restPtr);

int checkRuleHead(char *ruleHead, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc);

int checkRuleCondition(char *action, char *incond, char *args[MAX_NUM_OF_ARGS_IN_ACTION], 
		   int argc, ruleExecInfo_t *rei, int reiSaveFlag);

int replaceVariables(char *action, char *inStr, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		   ruleExecInfo_t *rei );
int replaceDollarParam(char *action, char *dPtr, int len, 
		   char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		   ruleExecInfo_t *rei);

int replaceArgVar( char *start, int size, 
	       char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc);

int replaceSessionVar(char *action,  char *start, int size,  ruleExecInfo_t *rei);

int getVarMap(char *action, char *varName, char **varMap, int index);

int getVarValue(char *varMap, ruleExecInfo_t *rei, char **varValue);

int replaceDataVar( char *start, int size,  dataObjInfo_t *doi);

int replaceUserVar( char *start, int size,  userInfo_t *uoic,  userInfo_t *uoip);

int replaceCollVar( char *start, int size,  collInfo_t *coi);

int replaceRescVar( char *start, int size,  rescInfo_t *roi);

int isNumber(char *s);

int isLogical(char *s);

int isAFunction(char *s);

int splitExpression(char *expr, char *expr1, char *expr2, char *oper);

int goodExpr(char *expr);

int replaceStrValue(char *start, int size, char *val, int paramLen);

int replaceIntValue(char *start, int size, int inval, int paramLen);

int replaceLongValue(char *start, int size, rodsLong_t inval, int paramLen);

int replaceULongValue(char *start, int size, rodsULong_t inval, int paramLen);

void *mallocAndZero(int s);

int copyRuleExecInfo(ruleExecInfo_t *from, ruleExecInfo_t *to);

int freeRuleExecInfoStruct(ruleExecInfo_t *rs, int freeSpeialStructFlag);

int freeRuleExecInfoInternals(ruleExecInfo_t *rs, int freeSpeialStructFlag);

int copyDataObjInfo(dataObjInfo_t *from, dataObjInfo_t *to);

int copyCollInfo(collInfo_t *from, collInfo_t *to);

int freeCollInfo(collInfo_t *rs);

int copyUserInfo(userInfo_t *from, userInfo_t *to);

int freeUserInfo(userInfo_t *rs);

int copyRescInfo(rescInfo_t *from, rescInfo_t *to);

int freeRescInfo(rescInfo_t *rs);

int copyRescGrpInfo(rescGrpInfo_t *from, rescGrpInfo_t *to);

int freeRescGrpInfo(rescGrpInfo_t *rs);

int copyKeyValPairStruct(keyValPair_t *from, keyValPair_t *to);

int freeKeyValPairStruct(keyValPair_t *rs);

int reREMatch(char *pat, char *str);

int trimWS (char *s);

int getValFromRuleExecInfo(char *varMap, ruleExecInfo_t *rei, char **varValue);

int getValFromRsComm(char *varMap, rsComm_t *ptr, char **varValue);

int getValFromDataObjInfo(char *varMap, dataObjInfo_t *ptr, char **varValue);

int getValFromRescInfo(char *varMap, rescInfo_t *ptr, char **varValue);

int getValFromUserInfo(char *varMap, userInfo_t *ptr, char **varValue);

int getValFromCollInfo(char *varMap, collInfo_t *ptr, char **varValue);

int getValFromKeyValPair(char *varMap, keyValPair_t *ptr, char **varValue);

int getValFromVersion(char *varMap, version_t *ptr, char **varValue);

int getValFromAuthInfo(char *varMap, authInfo_t *ptr, char **varValue);

int getValFromUserOtherInfo(char *varMap, userOtherInfo_t *ptr, char **varValue);

int getValFromDataObjInp(char *varMap, dataObjInp_t *ptr, char **varValue);

int getValFromDataOprInp(char *varMap, dataOprInp_t *ptr, char **varValue);

int getValFromFileOpenInp(char *varMap, fileOpenInp_t *ptr, char **varValue);

int getValFromRescGrpInfo(char *varMap, rescGrpInfo_t *ptr, char **varValue);

int getValFromRodsHostAddr(char *varMap, rodsHostAddr_t *ptr, char **varValue);

int mapExternalFuncToInternalProc( char *funcName);

int zeroRuleExecInfoStruct(ruleExecInfo_t *rei);
int initReiWithDataObjInp (ruleExecInfo_t *rei, rsComm_t *rsComm,
    dataObjInp_t *dataObjIn);
int initReiWithCollInp (ruleExecInfo_t *rei, rsComm_t *rsComm,
collInp_t *collCreateInp, collInfo_t *collInfo);
int packRei (rsComm_t *rsComm, ruleExecInfo_t *rei, bytesBuf_t **packedReiBBuf);
int unpackRei (rsComm_t *rsComm, ruleExecInfo_t **rei, bytesBuf_t *packedReiBBuf);
int touchupPackedRei (rsComm_t *rsComm, ruleExecInfo_t *myRei);
int packReiAndArg (rsComm_t *rsComm, ruleExecInfo_t *rei, char *myArgv[],
    int myArgc, bytesBuf_t **packedReiAndArgBBuf);
int unpackReiAndArg (rsComm_t *rsComm, ruleExecInfoAndArg_t **reiAndArg,
    bytesBuf_t *packedReiAndArgBBuf);

int pushStack(strArray_t *strArray, char *value);
int popStack(strArray_t *strArray, char *value);
int copyTaggedValue(char *str, char *tag, char *buf, int bufLen);
/***  causing trouble in compiling clientLogin.c
int
fillSubmitConditions (char *action, char *inDelayCondition, bytesBuf_t *packedReiAndArgBBuf,   ruleExecSubmitInp_t *ruleSubmitInfo);
***/
int print_uoi(userInfo_t *uoi);
int print_doi(dataObjInfo_t *doi);
#if defined(RULE_ENGINE_N)
int execMyRule(char * ruleDef,  msParamArray_t *inMsParamArray, char *outParamsDesc,
	  ruleExecInfo_t *rei);
int execMyRuleWithSaveFlag(char * ruleDef, msParamArray_t *inMsParamArray, char *outParamsDesc,
			   ruleExecInfo_t *rei,int reiSaveFlag);
#else
int execMyRule(char * ruleDef,  msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei);
int execMyRuleWithSaveFlag(char * ruleDef, msParamArray_t *inMsParamArray,
			   ruleExecInfo_t *rei,int reiSaveFlag);
#endif
int executeMyRuleBody(char *action, char *ruleAction, char *ruleRecovery, 
		msParamArray_t *inMsParamArray,
		ruleExecInfo_t *rei, int reiSaveFlag );
int executeMyRuleRecovery(char *ruleRecovery, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag);
int executeMyRuleAction(char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag);
int executeMicroService (char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	  ruleExecInfo_t *rei);
int applyActionCall(char *actionCall,  ruleExecInfo_t *rei, int reiSaveFlag);

int applyRuleArg(char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
	       ruleExecInfo_t *rei, int reiSaveFlag);
int applyRuleArgPA(char *action, char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
		      msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag);
int applyRule(char *inAction, msParamArray_t *inMsParamArray,
	  ruleExecInfo_t *rei, int reiSaveFlag);

int applyRuleForPostProcForRead(rsComm_t *rsComm, bytesBuf_t *dataObjReadOutBBuf, char *objPath);
int applyRuleForPostProcForWrite(rsComm_t *rsComm, bytesBuf_t *dataObjWriteOutBBuf, char *objPath);

int clearMsparamInRei (ruleExecInfo_t *rei);

int _delayExec(char *inActionCall, char *recoveryActionCall, 
	       char *delayCondition,  ruleExecInfo_t *rei);

int msiRollback(ruleExecInfo_t *rei);
int msiSetACL(msParam_t *recursiveFlag, msParam_t *accessLevel, msParam_t *userName, 
	      msParam_t *pathName, ruleExecInfo_t *rei);

int executeRuleActionNew(char *inAction,  msParamArray_t *inMsParamArray, 
    ruleExecInfo_t *rei, int reiSaveFlag);

int executeRuleRecoveryNew(char *ruleRecovery, msParamArray_t *inMsParamArray, 
    ruleExecInfo_t *rei, int reiSaveFlag);
int executeMicroServiceNew(char *inAction,  msParamArray_t *inMsParamArray,
                       ruleExecInfo_t *rei);
int makeAction(char *outAction,char *action, char *args[], int argc, int len);
int executeRuleBodyNew(char *action, char *ruleAction, char *ruleRecovery,
                msParamArray_t *inMsParamArray,
                ruleExecInfo_t *rei, int reiSaveFlag );
int replaceStarVar(char *action,  char *start, int size,   
     msParamArray_t *inMsParamArray);
int freeRuleArgs (char *args[], int argc);

int doForkExec(char *prog, char *arg1);
int writeString(msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei);
int getNewVarName(char *v, msParamArray_t *msParamArray);
int _writeString(char *writeId, char *writeStr, ruleExecInfo_t *rei);
int applyAllRules(char *inAction, msParamArray_t *inMsParamArray,
		  ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag);
int splitActions(char *expr, char *expr1, char **expr2);
int isStarVariable(char *str);

int setVarValue(char *varMap, ruleExecInfo_t *rei, void *newVarValue);
int getSetLeafValue(char **varValue, void *leafPtr, void **leafPtrVal, char *newVarValue, int setType);
int mapExternalFuncToInternalProc( char *funcName);
int getVarNameFromVarMap(char *varMap, char *varName, char **varMapCPtr);
int getSetValFromRuleExecInfo(char *varMap, ruleExecInfo_t **inrei, 
			      char **varValue, void *newVarValue);
int getSetValFromRsComm(char *varMap, rsComm_t **inptr, char **varValue, void *newVarValue);
int getSetValFromDataObjInfo(char *varMap, dataObjInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromDataObjInp(char *varMap, dataObjInp_t  **inptr, char **varValue, void *newVarValue);
int getSetValFromDataOprInp(char *varMap, dataOprInp_t  **inptr, char **varValue, void *newVarValue);
int getSetValFromRescInfo(char *varMap, rescInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromRescGrpInfo(char *varMap, rescGrpInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromUserInfo(char *varMap, userInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromCollInfo(char *varMap, collInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromKeyValPair(char *varMap, keyValPair_t **inptr, char **varValue, void *newVarValue);
int getSetValFromVersion(char *varMap, version_t **inptr, char **varValue, void *newVarValue);
int getSetValFromAuthInfo(char *varMap, authInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromUserOtherInfo(char *varMap, userOtherInfo_t **inptr, char **varValue, void *newVarValue);
int getSetValFromFileOpenInp(char *varMap, fileOpenInp_t **inptr, char **varValue, void *newVarValue);
int getSetValFromRodsHostAddr(char *varMap, rodsHostAddr_t **inptr, char **varValue, void *newVarValue);

int _admShowDVM(msParam_t *bufParam, ruleExecInfo_t *rei, rulevardef_t *inRuleVarDef, int inx);
int _admShowFNM(msParam_t *bufParam, ruleExecInfo_t *rei, rulefmapdef_t *inRuleFuncMapDef, int inx);
int _admShowIRB(msParam_t *bufParam, ruleExecInfo_t *rei, ruleStruct_t *inRuleStrct, int inx);


int getNextValueAndBufFromListOrStruct(char *typ, void *inPtr, void **value,  
				       bytesBuf_t **buf, void **restPtr, int *inx, char **outtyp);
int carryOverMsParam(msParamArray_t *sourceMsParamArray,msParamArray_t *targetMsParamArray);
int checkFilePerms(char *fileName);
int evaluateExpression(char *expr, char *res, ruleExecInfo_t *rei);
int removeTmpVarName(msParamArray_t *msParamArray);
int
getAllSessionVarValue (char *action, ruleExecInfo_t *rei,
keyValPair_t *varValues);
int
getSessionVarValue (char *action, char *varName, ruleExecInfo_t *rei,
char **varValue);
int insertRulesIntoDB(char * baseName, ruleStruct_t *coreRuleStruct,
		      ruleExecInfo_t *rei);
int writeRulesIntoFile(char * fileName, ruleStruct_t *myRuleStruct,
		   ruleExecInfo_t *rei);
int readRuleStructFromDB(char *ruleBaseName, char *versionStr, ruleStruct_t *inRuleStrct, 
			 ruleExecInfo_t *rei);
int insertDVMapsIntoDB(char * baseName, dvmStruct_t *coreDVMStruct,
		   ruleExecInfo_t *rei);
int writeDVMapsIntoFile(char * inFileName, dvmStruct_t *myDVMapStruct,
			ruleExecInfo_t *rei);
int readDVMapStructFromDB(char *dvmBaseName, char *versionStr, rulevardef_t *inDvmStrct, 
			  ruleExecInfo_t *rei);
int insertFNMapsIntoDB(char * baseName, fnmapStruct_t *coreFNMStruct,
		   ruleExecInfo_t *rei);
int writeFNMapsIntoFile(char * inFileName, fnmapStruct_t *myFNMapStruct,
			ruleExecInfo_t *rei);
int readFNMapStructFromDB(char *fnmBaseName, char *versionStr, fnmapStruct_t *inFnmStrct,
			  ruleExecInfo_t *rei);
int insertMSrvcsIntoDB(msrvcStruct_t *inMsrvcStruct,
		   ruleExecInfo_t *rei);
int readMsrvcStructFromDB(int inStatus, msrvcStruct_t *outMsrvcStrct, ruleExecInfo_t *rei);
int readMsrvcStructFromFile(char *msrvcFileName, msrvcStruct_t* inMsrvcStruct);
int writeMSrvcsIntoFile(char * inFileName, msrvcStruct_t *myMsrvcStruct,
		    ruleExecInfo_t *rei);


int _writeXMsg(int streamId, char *hdr, char *msg);
int _readXMsg(int streamId, char *contRead, int *msgNum, int *seqNum, char **hdr, char **msg,char **user, char **addr);
#ifdef RULE_ENGINE_N
int reDebug(char *callLabel, int flag, char *action, char *actionStr, Node *node, Env *env, ruleExecInfo_t *rei);
void disableReDebugger(int state[2]);
void enableReDebugger(int state[2]);
#else
int reDebug(char *callLabel, int flag, char *actionStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei);
#endif
int initializeReDebug(rsComm_t *svrComm, int flag);


int _applyRule(char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag);
int _applyAllRules(char *inAction, msParamArray_t *inMsParamArray,
		   ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag);
int _execMyRuleWithSaveFlag(char * ruleDef, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei,int reiSaveFlag);


#endif  /* RE_FUNCDEFS_H */
