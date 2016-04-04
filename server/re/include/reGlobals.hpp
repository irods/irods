/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reGlobals.h - header file for globals in the rule engine
 */
#ifndef RE_GLOBALS_HPP
#define RE_GLOBALS_HPP


#include "reGlobalsExtern.hpp"

ruleStruct_t coreRuleStrct;
rulevardef_t coreRuleVarDef;
rulefmapdef_t coreRuleFuncMapDef;
ruleStruct_t appRuleStrct;
rulevardef_t appRuleVarDef;
rulefmapdef_t appRuleFuncMapDef;
msrvcStruct_t coreMsrvcStruct;
msrvcStruct_t appMsrvcStruct;
int reTestFlag = 0;
int reLoopBackFlag = 0;
int GlobalAllRuleExecFlag = 0;
int GlobalREDebugFlag = 0;
int GlobalREAuditFlag = 0;
char *reDebugStackFull[REDEBUG_STACK_SIZE_FULL];
struct reDebugStack reDebugStackCurr[REDEBUG_STACK_SIZE_CURR];
int reDebugStackFullPtr = 0;
int reDebugStackCurrPtr = 0;

char tmpStr[MAX_ERROR_STRING];
strArray_t delayStack;
strArray_t msParamStack;
#endif  /* RE_GLOBALS_H */
