/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reGlobalsExtern.hpp - header file for global extern declaration for the
 * rule engine modules
 */

#ifndef RE_GLOBALS_EXTERN_HPP
#define RE_GLOBALS_EXTERN_HPP

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

#include "irods/rodsUser.h"
#include "irods/rods.h"
#include "irods/rcGlobalExtern.h"
#include "irods/objInfo.h"
#include "irods/fileOpen.h"
#include "irods/reDefines.h"
#include "irods/ruleExecSubmit.h"
#include "irods/ruleExecDel.h"
#include "irods/dataObjInpOut.h"
#include "irods/msParam.h"
#include "irods/modAccessControl.h"

#if 0
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
/***** If you are changing the RuleExecInfo   *****/
/***** You need to modify the maintenance     *****/
/***** functions. Please refer to the file    *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** ALSO, if any structure in RuleExecInfo *****/
/***** has its definition changed there need  *****/
/***** corresponding changes in maintenance   *****/
/***** files. Please refer to the  file       *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
typedef struct RuleExecInfo {
    int status;
    char statusStr[MAX_NAME_LEN];
    char ruleName[NAME_LEN];	/* name of rule */
    rsComm_t *rsComm;
    char pluginInstanceName[MAX_NAME_LEN];
    msParamArray_t *msParamArray;
    msParamArray_t inOutMsParamArray;
    int l1descInx;
    dataObjInp_t *doinp;	/* data object type input */
    dataObjInfo_t *doi;
    char rescName[NAME_LEN]; // replaces rgi above
    userInfo_t *uoic;  /* client XXXX should get this from rsComm->clientUser */
    userInfo_t *uoip;  /* proxy XXXX should get this from rsComm->proxyUser */
    collInfo_t *coi;
    userInfo_t *uoio;     /* other user info */
    keyValPair_t *condInputData;
    /****        IF YOU ARE MAKING CHANGES CHECK BELOW
                 OR ABOVE FOR IMPORTANT INFORMATION  ****/
    char ruleSet[RULE_SET_DEF_LENGTH];
    struct RuleExecInfo *next;
} ruleExecInfo_t;
#endif
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/
/***** If you are changing the RuleExecInfo   *****/
/***** You need to modify the maintenance     *****/
/***** functions. Please refer to the file    *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** ALSO, if any structure in RuleExecInfo *****/
/***** has its definition changed there need  *****/
/***** corresponding changes in maintenance   *****/
/***** files. Please refer to the  file       *****/
/***** WhatToDoWhenYouChangeREIStructure.txt  *****/
/***** for more details.                      *****/
/***** IMPORTANT    IMPORTANT    IMPORTANT    *****/

struct reDebugStack {
    char *step;
    int label;
};

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
extern struct reDebugStack reDebugStackCurr[REDEBUG_STACK_SIZE_CURR];
extern int reDebugStackFullPtr;
extern int reDebugStackCurrPtr;

//#include "irods/private/re/reFuncDefs.hpp"
#include "irods/private/re/reHelpers1.hpp"
#endif  /* RE_GLOBALS_EXTERN_H */
