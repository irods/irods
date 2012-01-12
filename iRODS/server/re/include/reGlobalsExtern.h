/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reGlobalsExtern.h - header file for global extern declaration for the 
 * rule engine modules
 */

#ifndef RE_GLOBALS_EXTERN_H
#define RE_GLOBALS_EXTERN_H

/****
#ifdef MALLOC_TESTING
/ * Include the following code to log each malloc and free.
   This can be useful in testing/debugging of memory allocation problems. * /
#define MYMALLOC 1
#define malloc(x) mymalloc(__FILE__, __LINE__ , x)
extern void* mymalloc(char* file, int line, int x); 
#define free(x) myfree(__FILE__, __LINE__ , x)
extern void myfree(char* file, int line, void *x); 
#endif
***/

#include "rodsUser.h"
#include "rods.h"
#include "rcGlobalExtern.h"
#include "objInfo.h"
#include "fileOpen.h"
#include "regExpMatch.h"
#include "reDefines.h"
#include "ruleExecSubmit.h"
#include "ruleExecDel.h"
#include "dataObjInpOut.h"
#include "msParam.h"
#include "modAccessControl.h"

/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
/***** If you are changing the RuleExecInfo   *****/
/***** You need to modify the maintenance     *****/
/***** functions. Please refer to the file    *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** ALSO, if any structure in RuleExecInfo *****/
/***** has its definition changed there need  *****/
/***** correspomding changes in maintenace    *****/
/***** files. Please refer to the  file       *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
typedef struct RuleExecInfo {
  int status;
  char statusStr[MAX_NAME_LEN];
  char ruleName[NAME_LEN];	/* name of rule */
  rsComm_t *rsComm;
  msParamArray_t *msParamArray;
  msParamArray_t inOutMsParamArray;
  int l1descInx;
  dataObjInp_t *doinp;	/* data object type input */
#if 0
  dataOprInp_t *dinp;   /* XXXXX - not used. data input */
  fileOpenInp_t *finp;  /* XXXXX - not used. file type input */
#endif
  dataObjInfo_t *doi;
  rescGrpInfo_t *rgi; /* resource group */
  userInfo_t *uoic;  /* client XXXX should get this from rsComm->clientUser */
  userInfo_t *uoip;  /* proxy XXXX should get this from rsComm->proxyUser */
  collInfo_t *coi;
#if 0
  dataObjInp_t *doinpo;  /* other data object type input. for copy operation */
  dataOprInp_t *dinpo;   /* other data type input. for copy operation */
  fileOpenInp_t *finpo;  /* other file type input. for copy operation */
  rescGrpInfo_t *rgio; /* other resource group. for copy operation */
#endif
  userInfo_t *uoio;     /* other user info */
  keyValPair_t *condInputData;
  /****        IF YOU ARE MAKING CHANGES CHECK BELOW
               OR ABOVE FOR IMPORTANT INFORMATION  ****/
  char ruleSet[RULE_SET_DEF_LENGTH];
  struct RuleExecInfo *next;
} ruleExecInfo_t;
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
/***** If you are changing the RuleExecInfo   *****/
/***** You need to modify the maintenance     *****/
/***** functions. Please refer to the file    *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** ALSO, if any structure in RuleExecInfo *****/
/***** has its definition changed there need  *****/
/***** correspomding changes in maintenace    *****/
/***** files. Please refer to the  file       *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/

typedef struct ReArg {
    int myArgc;
    char **myArgv;
} reArg_t;

typedef struct RuleExecInfoAndArg {
    ruleExecInfo_t *rei;
    reArg_t reArg;
} ruleExecInfoAndArg_t;

typedef struct {
  int MaxNumOfRules;
  char    *ruleBase[MAX_NUM_OF_RULES];
  char    *action[MAX_NUM_OF_RULES];
  char    *ruleHead[MAX_NUM_OF_RULES];
  char    *ruleCondition[MAX_NUM_OF_RULES];
  char    *ruleAction[MAX_NUM_OF_RULES];
  char    *ruleRecovery[MAX_NUM_OF_RULES];
  long int      ruleId[MAX_NUM_OF_RULES]; 
} ruleStruct_t;

typedef struct {
  int MaxNumOfDVars;
  char *varName[MAX_NUM_OF_DVARS];
  char *action[MAX_NUM_OF_DVARS];
  char *var2CMap[MAX_NUM_OF_DVARS];
  long int   varId[MAX_NUM_OF_DVARS];
} rulevardef_t;

typedef rulevardef_t dvmStruct_t;

typedef struct {
  int MaxNumOfFMaps;
  char *funcName[MAX_NUM_OF_FMAPS];
  char *func2CMap[MAX_NUM_OF_FMAPS];
  long int  fmapId[MAX_NUM_OF_FMAPS];
} rulefmapdef_t;

typedef rulefmapdef_t fnmapStruct_t;

typedef struct {
  int MaxNumOfMsrvcs;
  long int   msrvcId[MAX_NUM_OF_MSRVCS];
  char    *moduleName[MAX_NUM_OF_MSRVCS];
  char    *msrvcName[MAX_NUM_OF_MSRVCS];
  char    *msrvcSignature[MAX_NUM_OF_MSRVCS];
  char    *msrvcVersion[MAX_NUM_OF_MSRVCS];
  char    *msrvcHost[MAX_NUM_OF_MSRVCS];
  char    *msrvcLocation[MAX_NUM_OF_MSRVCS];
  char    *msrvcLanguage[MAX_NUM_OF_MSRVCS];
  char    *msrvcTypeName[MAX_NUM_OF_MSRVCS];
  long int   msrvcStatus[MAX_NUM_OF_MSRVCS];
} msrvcStruct_t;

extern ruleStruct_t coreRuleStrct;
extern rulevardef_t coreRuleVarDef;
extern rulefmapdef_t coreRuleFuncMapDef;
extern  msrvcStruct_t coreMsrvcStruct;
extern ruleStruct_t appRuleStrct;
extern rulevardef_t appRuleVarDef;
extern rulefmapdef_t appRuleFuncMapDef;
extern  msrvcStruct_t appMsrvcStruct;
extern int reTestFlag;
extern int reLoopBackFlag;
extern int GlobalREDebugFlag;
extern int GlobalREAuditFlag;
extern char *reDebugStackFull[REDEBUG_STACK_SIZE_FULL];
#ifdef RULE_ENGINE_N
extern char *reDebugStackCurr[REDEBUG_STACK_SIZE_CURR][3];
#else
extern char *reDebugStackCurr[REDEBUG_STACK_SIZE_CURR];
#endif
extern int reDebugStackFullPtr;
extern int reDebugStackCurrPtr;

extern char tmpStr[];
extern strArray_t delayStack;
extern strArray_t msParamStack;

#include "reFuncDefs.h"
#include "reHelpers1.h"
#endif  /* RE_GLOBALS_EXTERN_H */
