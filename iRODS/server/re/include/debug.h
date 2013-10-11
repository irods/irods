/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef DEBUG_H
#define DEBUG_H
#ifdef RULE_ENGINE_N
#include "restructs.h"
int pushReStack(RuleEngineEvent label, char* step);
int popReStack(RuleEngineEvent label, char *step);
int reDebug(RuleEngineEvent callLabel, int flag, RuleEngineEventParam *param, Node *node, Env *env, ruleExecInfo_t *rei);
#else
int reDebug(char *callLabel, int flag, char *actionStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei);
#endif
int initializeReDebug(rsComm_t *svrComm, int flag);

#endif
