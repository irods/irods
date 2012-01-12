/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef RULES_H
#define RULES_H
#include "parser.h"
#include "utils.h"
#include "arithmetics.h"
#include "typing.h"

int setLocalVarValue(char* varName, ruleExecInfo_t *rei, Res* res, char* errmsg, Region *r);
int readRuleSetFromFile(char *ruleBaseName, RuleSet *ruleSet, Env *funcDesc, int* errloc, rError_t *errmsg, Region *r);
int readRuleSetFromLocalFile(char *ruleBaseName, char *fileName, RuleSet *ruleSet, Env *funcDesc, int *errloc, rError_t *errmsg, Region *r);
int parseAndComputeMsParamArrayToEnv(msParamArray_t *msParamArray, Env *global, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
int parseAndComputeRule(char *expr, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
int parseAndComputeRuleNewEnv( char *expr, ruleExecInfo_t *rei, int reiSaveFlag, msParamArray_t *msParamArray, rError_t *errmsg, Region *r);
int parseAndComputeRuleAdapter(char *rule, msParamArray_t *msParamArray, ruleExecInfo_t *rei, int reiSaveFlag, Region *r);
Res *parseAndComputeExpression(char * expr,Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res *parseAndComputeExpressionAdapter(char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag, Region *r);
Res *computeExpressionWithParams(char *actionName, char** params, int paramCount, ruleExecInfo_t *rei, int reiSaveFlag, msParamArray_t *vars, rError_t *errmsg, Region *r);
Res *computeNode(Node *expr, Node *reco, Env *env, ruleExecInfo_t *rei, int reiSaveFlag , rError_t *errmsg, Region *r);

ExprType *typeRule(RuleDesc *ruleNode, Env *funcDesc, Hashtable *varTypes, List *typingConstraints, rError_t *errmsg, Node **errnode, Region *r);
ExprType *typeRuleSet(RuleSet *ruleset, rError_t *errmsg, Node **errnode, Region *r);
execCmdOut_t *addCmdExecOutToEnv(Env *global, Region *r);
void freeCmdExecOut(execCmdOut_t *ruleExecOut);
RuleDesc *getRuleDesc(int ri);
int generateRuleTypes(RuleSet *inRuleSet, Hashtable *symbol_type_table, Region *r);
int actionTableLookUp (char *action);

int overflow(char*expr,int len);
Env *defaultEnv(Region *r);

#endif
