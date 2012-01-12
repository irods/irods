/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef TYPING_H
#define TYPING_H

#include "utils.h"
#include "hashtable.h"
#include "region.h"
#include "parser.h"

typedef enum satisfiability {
    TAUTOLOGY = 1, CONTINGENCY = 2, ABSURDITY = 4
} Satisfiability;

ExprType * typeExpression3(Node *expr, int notyping, Env *funcDesc, Hashtable *varTypes, List *typingConstraints, rError_t* errmsg, Node **errnode, Region *r);
int typeFuncParam(Node *param, Node *paramType, Node *formalParamType, Hashtable *var_type_table, List *typingConstraints, rError_t *errmsg, Region *r);
void postProcessActions(Node *expr, Env *systemFunctionTables, rError_t *errmsg, Node **errnode, Region *r);
void postProcessCoercion(Node *expr, Hashtable *varTypes, rError_t* errmsg, Node **errnode, Region *r);
int isBaseType(ExprType *t);
int tautologyLtBase(ExprType *a, ExprType *b);
int tautologyLt(ExprType *type, ExprType *expected);
int consistent(List *typingConstraints, Hashtable *typingEnv, Region *r);
Satisfiability splitBaseType(ExprType *tca, ExprType *tcb, int flex);
Satisfiability splitVarL(ExprType *var, ExprType *consTuple, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability splitVarR(ExprType *consTuple, ExprType *var, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability splitConsOrTuple(ExprType *a, ExprType *b,int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability simplifyR(ExprType *a, ExprType *b,int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability simplifyL(ExprType *a, ExprType *b,int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability narrow(ExprType *a, ExprType *b,int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);
Satisfiability simplifyLocally(ExprType *a, ExprType *b, int flex, Node *node, Hashtable *typingEnv, Hashtable *equivalence, List *simpleTypingConstraints, Region *r);

Satisfiability simplify(List *typingConstraints, Hashtable *typingEnv, rError_t* errmsg, Node **errnode, Region *r);
Satisfiability solveConstraints(List *typingConstraints, Hashtable *typingEnv, rError_t* errmsg, Node **errnode, Region *r);
ExprType *replaceDynamicWithNewTVar(ExprType *type, Region *r);
#endif /* TYPING_H */
