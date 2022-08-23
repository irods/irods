/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef RE_FUNCDEFS_HPP
#define RE_FUNCDEFS_HPP

#include "irods/private/re/reGlobalsExtern.hpp"
#include "irods/private/re/reHelpers1.hpp"
#include "irods/irods_re_structs.hpp"
int initRuleStruct( const char* inst_name, rsComm_t *svrComm, const char *ruleSet, const char *dvmSet, const char *fnmSet );

int readRuleStructFromFile( const char* inst_name, const char *ruleBaseName );

int clearRuleStruct( ruleStruct_t *inRuleStrct );

int readDVarStructFromFile( char *dvarBaseName, rulevardef_t *inRuleVarDef );

int clearDVarStruct( rulevardef_t *inRuleVarDef );

int readFuncMapStructFromFile( char *fmapBaseName, rulefmapdef_t* inRuleFuncMapDef );

int clearFuncMapStruct( rulefmapdef_t* inRuleFuncMapDef );

int getRule( int ri, char *ruleBase, char *ruleHead, char *ruleCondition,
             char *ruleAction, char *ruleRecovery, int rSize );

#include "irods/irods_ms_plugin.hpp"
int actionTableLookUp( irods::ms_table_entry&, char *action );

int applyRuleArgPA( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
                    msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag );
int applyRuleBase( char *inAction, msParamArray_t *inMsParamArray, int update,
                   ruleExecInfo_t *rei, int reiSaveFlag );
int applyRuleUpdateParams( char *inAction, msParamArray_t *inMsParamArray,
                           ruleExecInfo_t *rei, int reiSaveFlag ); // JMC - backport 4538
int applyRuleUpdateParams( char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ); // JMC - backport 4539

int applyAllRules( char *inAction, msParamArray_t *inMsParamArray,
                   ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag );

#if 0	// #1472
int copyRescInfo( rescInfo_t *from, rescInfo_t *to );
int freeRescInfo( rescInfo_t *rs );
int copyRescGrpInfo( rescGrpInfo_t *from, rescGrpInfo_t *to );
int freeRescGrpInfo( rescGrpInfo_t *rs );
#endif

int mapExternalFuncToInternalProc( char *funcName );

int zeroRuleExecInfoStruct( ruleExecInfo_t *rei );
int packRei( ruleExecInfo_t *rei, bytesBuf_t **packedReiBBuf );
int unpackRei( rsComm_t *rsComm, ruleExecInfo_t **rei, bytesBuf_t *packedReiBBuf );
int packReiAndArg( ruleExecInfo_t *rei, char *myArgv[],
                   int myArgc, bytesBuf_t **packedReiAndArgBBuf );
int unpackReiAndArg( rsComm_t *rsComm, ruleExecInfoAndArg_t **reiAndArg,
                     bytesBuf_t *packedReiAndArgBBuf );

int pushStack( strArray_t *strArray, char *value );
int popStack( strArray_t *strArray, char *value );
std::map<std::string, std::vector<std::string>> getTaggedValues(const char *str);
/***  causing trouble in compiling clientLogin.c
int
fillSubmitConditions (const char *action, const char *inDelayCondition, bytesBuf_t *packedReiAndArgBBuf,   ruleExecSubmitInp_t *ruleSubmitInfo);
***/
int print_uoi( userInfo_t *uoi );
int print_doi( dataObjInfo_t *doi );
int execMyRule( char * ruleDef,  msParamArray_t *inMsParamArray, const char *outParamsDesc,
                ruleExecInfo_t *rei );
int execMyRuleWithSaveFlag( char * ruleDef, msParamArray_t *inMsParamArray, const char *outParamsDesc,
                            ruleExecInfo_t *rei, int reiSaveFlag );

int clearMsparamInRei( ruleExecInfo_t *rei );

int _delayExec( const char *inActionCall, const char *recoveryActionCall,
                const char *delayCondition,  ruleExecInfo_t *rei );

int doForkExec( char *prog, char *arg1 );
int writeString( msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei );
int getNewVarName( char *v, msParamArray_t *msParamArray );

int mapExternalFuncToInternalProc( char *funcName );

int _admShowDVM( ruleExecInfo_t *rei, rulevardef_t *inRuleVarDef, int inx );
int _admShowFNM( ruleExecInfo_t *rei, rulefmapdef_t *inRuleFuncMapDef, int inx );

int carryOverMsParam( msParamArray_t *sourceMsParamArray, msParamArray_t *targetMsParamArray );
int checkFilePerms( char *fileName );
int removeTmpVarName( msParamArray_t *msParamArray );
int
getAllSessionVarValue( ruleExecInfo_t *rei, keyValPair_t *varValues );
int
getSessionVarValue( char *action, char *varName, ruleExecInfo_t *rei,
                    char **varValue );
int insertRulesIntoDB( char * baseName, ruleStruct_t *coreRuleStruct,
                       ruleExecInfo_t *rei );
int writeRulesIntoFile( char * fileName, ruleStruct_t *myRuleStruct,
                        ruleExecInfo_t *rei );
int readRuleStructFromDB( char *ruleBaseName, char *versionStr, ruleStruct_t *inRuleStrct,
                          ruleExecInfo_t *rei );
int insertDVMapsIntoDB( char * baseName, dvmStruct_t *coreDVMStruct,
                        ruleExecInfo_t *rei );
int writeDVMapsIntoFile( char * inFileName, dvmStruct_t *myDVMapStruct,
                         ruleExecInfo_t *rei );
int readDVMapStructFromDB( char *dvmBaseName, char *versionStr, rulevardef_t *inDvmStrct,
                           ruleExecInfo_t *rei );
int insertFNMapsIntoDB( char * baseName, fnmapStruct_t *coreFNMStruct,
                        ruleExecInfo_t *rei );
int writeFNMapsIntoFile( char * inFileName, fnmapStruct_t *myFNMapStruct,
                         ruleExecInfo_t *rei );
int readFNMapStructFromDB( char *fnmBaseName, char *versionStr, fnmapStruct_t *inFnmStrct,
                           ruleExecInfo_t *rei );
int insertMSrvcsIntoDB( msrvcStruct_t *inMsrvcStruct,
                        ruleExecInfo_t *rei );
int readMsrvcStructFromDB( int inStatus, msrvcStruct_t *outMsrvcStrct, ruleExecInfo_t *rei );
int readMsrvcStructFromFile( char *msrvcFileName, msrvcStruct_t* inMsrvcStruct );
int writeMSrvcsIntoFile( char * inFileName, msrvcStruct_t *myMsrvcStruct,
                         ruleExecInfo_t *rei );

void disableReDebugger( int state[2] );
void enableReDebugger( int state[2] );

#endif  /* RE_FUNCDEFS_H */
