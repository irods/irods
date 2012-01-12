/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef ARITHMETICS_H
#define ARITHMETICS_H
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "conversion.h"
#include "parser.h"
#include "hashtable.h"
#define POSIX_REGEXP

#ifndef DEBUG

#include "msParam.h"
#include "rodsDef.h"

#endif

#define RETURN {goto ret;}

/** AST evaluators */
Res* evaluateActions(Node *ruleAction, Node *ruleRecovery,
                   int applyAll, ruleExecInfo_t *rei, int reiSaveFlag , Env *env,
                   rError_t *errmsg, Region *r);
Res* evaluateExpression3(Node *node, int applyAll, int force, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r);
Res* attemptToEvaluateVar3(char* vn, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r);
Res* evaluateVar3(char* vn, Node *node, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r);
Res *evaluateFunctionApplication(Node *func, Node *arg, int applyAll, Node *node, ruleExecInfo_t* rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r);
Res* evaluateFunction3(Node* appNode, int applyAll, Node *astNode, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res* execAction3(char *fn, Res** args, unsigned int nargs, int applyAll, Node *node, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res* execMicroService3 (char *inAction, Res** largs, unsigned int nargs, Node *node, Env *env, ruleExecInfo_t *rei, rError_t *errmsg, Region *r);
Res* execRule(char *ruleName, Res** args, unsigned int narg, int applyAll, Env *outEnv, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res* execRuleNodeRes(Node *rule, Res** args, unsigned int narg, int applyAll, Env *outEnv, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res* matchPattern(Node *pattern, Node *val, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);
Res* matchPattern(Node *pattern, Node *val, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r);

Res* getSessionVar(char *action,  char *varName,  ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r);
Res* processCoercion(Node *node, Res *res, ExprType *type, Hashtable *tvarEnv, rError_t *errmsg, Region *r);

/** utilities */
char* getVariableName(Node *node);
char* matchWholeString(char *buf);
char *wildCardToRegex(char *buf);
void copyFromEnv(Res**params, char **paramNames, int paramsCount, Hashtable *env, Region *r);
int initializeEnv(Node *ruleHead, Res **args, int argc, Hashtable *env, Region *r);



#endif
