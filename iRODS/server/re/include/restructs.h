/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef RESTRUCTS_H
#define RESTRUCTS_H
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reGlobalsExtern.h"
#ifndef DEBUG
#include "rodsType.h"
#include "msParam.h"
#endif

#include "reconstants.h"
#include "region.h"
#include "hashtable.h"
#include "list.h"

#define TYPE(x) ((x)->exprType->nodeType)

#define T_CONS_TYPE_ARGS(x) ((x)->subtrees)
#define T_CONS_TYPE_ARG(x, n) ((x)->subtrees[n])
#define T_CONS_TYPE_NAME(x) ((x)->text)
#define T_CONS_ARITY(x) ((x)->degree)
#define T_FUNC_PARAM_TYPE(x, n) (T_CONS_TYPE_ARG((x)->subtrees[0], n))
#define T_FUNC_PARAM_TYPE_VARARG(x, n) (n<T_FUNC_ARITY(x)?T_FUNC_PARAM_TYPE(x,n):T_FUNC_PARAM_TYPE(x,T_FUNC_ARITY(x)-1))
#define T_FUNC_RET_TYPE(x) ((x)->subtrees[1])
#define T_FUNC_ARITY(x) ((x)->subtrees[0]->degree)
#define T_FUNC_VARARG(x) ((x)->vararg)
#define T_VAR_ID(x) ((x)->ival)
#define T_VAR_DISJUNCT(x, n) ((x)->subtrees[n])
#define T_VAR_DISJUNCTS(x) ((x)->subtrees)
#define T_VAR_NUM_DISJUNCTS(x) ((x)->degree)
#define TC_A(tc) ((tc)->subtrees[0])
#define TC_B(tc) ((tc)->subtrees[1])
#define TC_NODE(tc) ((tc)->subtrees[2])
#define TC_NEXT(tc) ((tc)->subtrees[3])
#define N_APP_ARG(x, n) ((x)->subtrees[1]->subtrees[n])
#define N_APP_FUNC(x) ((x)->subtrees[0])
#define N_APP_ARITY(x) ((x)->subtrees[1]->degree)
#define RULE_NODE_NUM_PARAMS(r) ((r)->subtrees[0]->subtrees[0]->degree)
#define RULE_NAME(r) ((r)->subtrees[0]->text)

#define FD_PROJ(x) (x)->ival
#define RES_FUNC_N_ARGS(x) (x)->ival
#define RES_ERR_CODE(x) (x)->ival
#define RES_FUNC_N_ARGS(x) (x)->ival
#define N_TUPLE_CONSTRUCT_TUPLE(x) (x)->ival
#define RES_STRING_STR_LEN(x) (x)->ival
#define RES_DOUBLE_VAL_LVAL(x) ((x)->dval)
#define RES_INT_VAL_LVAL(x) ((x)->dval)
#define RES_BOOL_VAL_LVAL(x) ((x)->dval)
#define RES_DOUBLE_VAL(x) ((x)->dval)
#define RES_INT_VAL(x) ((int)(x)->dval)
#define RES_BOOL_VAL(x) ((int)(x)->dval)
#define RES_TIME_VAL(x) ((x)->lval)
#define FD_RULE_INDEX_LIST(x) ((x)->ruleIndexList)
#define FD_RULE_INDEX_LIST_LVAL(x) FD_RULE_INDEX_LIST(x)
#define FD_SMSI_FUNC_PTR(x) ((x)->func)
#define FD_SMSI_FUNC_PTR_LVAL(x) FD_SMSI_FUNC_PTR(x)
#define RES_UNINTER_STRUCT(x) ((x)->param->inOutStruct)
#define RES_UNINTER_BUFFER(x) ((x)->param->inpOutBuf)
#define RES_IRODS_TYPE(x) ((x)->param->type)
#define NODE_EXPR_POS(x) ((x)->expr)
#define getNodeType(x) ((NodeType)((x)->nodeType))
#define setNodeType(x, t) ((x)->nodeType = (int) t)

#define OPTION_VARARG_ONCE 0x0
#define OPTION_VARARG_STAR 0x1
#define OPTION_VARARG_PLUS 0x2
#define OPTION_VARARG_OPTIONAL 0x3
#define OPTION_VARARG_MASK 0xf
#define OPTION_COERCE 0x10
#define OPTION_TYPED 0x20

#define OPTION_IO_TYPE_MASK 0xff00
#define IO_TYPE_INPUT 0x100
#define IO_TYPE_OUTPUT 0x200
#define IO_TYPE_DYNAMIC 0x400
#define IO_TYPE_EXPRESSION 0x800
#define IO_TYPE_ACTIONS 0x1000

#define getVararg(n) ((n)->option & OPTION_VARARG_MASK)
#define setVararg(n, v) (n)->option ^= ((n)->option & OPTION_VARARG_MASK) ^ (v);
#define getIOType(n) ((n)->option & OPTION_IO_TYPE_MASK)
#define setIOType(n, v) (n)->option ^= ((n)->option & OPTION_IO_TYPE_MASK) ^ (v);

typedef struct node Node;
typedef struct node ExprType;
typedef struct node Res;
typedef struct node FunctionDesc;
typedef struct node TypingConstraint;
typedef struct node *NodePtr;

typedef char *charPtr;
typedef ExprType *ExprTypePtr;
typedef struct bucket Bucket;
typedef Bucket *BucketPtr;
typedef msParam_t *msParam_tPtr;


typedef enum node_type {
    TK_EOS = -1,
    N_ERROR = 0,
    TK_INT = 1,
    TK_DOUBLE = 2,
    TK_TEXT = 3,
    TK_STRING = 4,
    TK_BOOL = 5,
    TK_BACKQUOTED = 6,
    TK_DATETIME = 7, /* unused */
    TK_VAR = 8,
    TK_LOCAL_VAR = 10,
    TK_SESSION_VAR = 11,
    TK_OP = 12,
    TK_MISC_OP = 14,
    TK_COL = 16,
    TK_PATH = 18,
    N_VAL = 20,
    N_TUPLE = 21,
    N_APPLICATION = 22,
    N_PARTIAL_APPLICATION = 23,
    N_ATTR = 25,
    N_QUERY_COND = 26,
    N_QUERY_COND_JUNCTION = 27,
    N_QUERY = 28,
    N_ACTIONS = 30,
    N_ACTIONS_RECOVERY = 31,
    N_RULE_NAME = 32,
    N_PARAM_LIST = 33,
    N_PARAM_TYPE_LIST = 34,
    N_AVU = 35,
    N_META_DATA = 36,
    N_RULE_PACK = 37,
    N_RULESET = 38,
    N_FD_FUNCTION = 41,
    N_FD_CONSTRUCTOR = 42,
    N_FD_DECONSTRUCTOR = 43,
    N_FD_EXTERNAL = 44,
    N_FD_RULE_INDEX_LIST = 45,
    N_SYM_LINK = 46,
    N_RULE_CODE = 50,
    N_RULE = 60,
    N_CONSTRUCTOR_DEF = 61,
    N_EXTERN_DEF = 62,
    N_DATA_DEF = 63,
    N_UNPARSED = 64,
    /* K_FLEX = 90, */
    T_UNSPECED = 100, /* indicates a variable which is not assigned a value is passed in to a microservice */
    T_ERROR = 101,
    T_DYNAMIC = 200,
    T_DOUBLE = 201,
    T_INT = 202,
    T_STRING = 203,
    T_DATETIME = 204,
    T_BOOL = 205,
    T_FLEX = 206,
    T_FIXD = 207,
    T_TUPLE = 208,
    T_CONS = 209,
    T_PATH = 220,
    T_BREAK = 230,
    T_SUCCESS = 231,
    T_VAR = 300,
    T_IRODS = 400,
    T_TYPE = 500,
    TC_LT = 600,
    TC_SET = 660,
    PI_BIN = 1002,
    PI_CHAR = 1008,
    PI_STR = 1009,
    PI_PISTR = 1010,
    PI_INT16 = 1016,
    PI_INT = 1032,
    PI_DOUBLE = 1064,
    PI_POINTER = 2001,
    PI_STRUCT = 2002,
    PI_DEPENDENT = 2003,
    PI_INT_DEPENDENT = 2004,
    PI_DIM = 2006,
    PI_MEMBER = 2007,
    PI_INDEX = 2008,
    PI_CASE = 2009,
    PI_DEFAULT = 2010,
    PI_TYPE = 2011,
    PI_ARRAY_MEMBER = 2012,
    C_BASE_TYPE = 2020,
    C_POINTER_TYPE = 2021,
    C_ARRAY_TYPE = 2022,
    C_ARRAY_TYPE_DIM = 2023,
    C_STRUCT_TYPE = 2024,
    C_STRUCT_MEMBER = 2025,
    C_STRUCT_DEF = 2026,
    C_DEF_SET = 2030,
    C_DEF_SET_SET = 2031,
    CG_ANNOTATION = 2032,
    CG_ANNOTATIONS = 2033,
} NodeType;

typedef struct condIndexVal {
    Node *params;
    Node *condExp;
    Hashtable *valIndex; /* char * -> int * */
} CondIndexVal;

typedef struct ruleIndexListNode {
    struct ruleIndexListNode *next, *prev;
    int secondaryIndex;
	int ruleIndex;
	CondIndexVal *condIndex;
} RuleIndexListNode;

typedef struct ruleIndexList {
    char *ruleName;
    RuleIndexListNode *head, *tail;
} RuleIndexList;

typedef struct env Env;
struct env {
    Hashtable *current;
    Env *previous;
    Env *lower;
};

typedef Res *(SmsiFuncType)(Node **, int, Node *, ruleExecInfo_t *, int, Env *, rError_t *, Region *);
typedef SmsiFuncType *SmsiFuncTypePtr;



typedef struct str_list {
    char *str;
    struct str_list *next;
} StringList;



struct node {
    int nodeType; /* node type */
    int degree;
    int option; /* weather runtime coercion is needed */
    int ival;
    /* when this node represents a type or a pattern, this field indicates whether the trailing subtree represents varargs */
    ExprType *exprType; /* expression type */
    ExprType *coercionType; /* coercion type */
    char *text;
    rodsLong_t expr;
    struct node **subtrees;
    char *base;
    double dval;
    rodsLong_t lval;
    RuleIndexList *ruleIndexList;
    SmsiFuncTypePtr func;
	msParam_t *param;
};

typedef enum ruleType {
    RK_REL,
    RK_FUNC,
    RK_DATA,
    RK_CONSTRUCTOR,
    RK_EXTERN,
    /* RK_UNPARSED, */
} RuleType;

typedef struct {
    int id;
    Node *type;
    Node *node;
    RuleType ruleType;
    int dynamictyping;
} RuleDesc;

typedef RuleDesc *RuleDescPtr;

typedef struct ruleSet {
	int len;
	RuleDesc* rules[MAX_NUM_RULES];
	/* Region *region; */
} RuleSet;

typedef struct label {
    long exprloc;
    char *base;
} Label;

typedef struct token {
    NodeType type;
    char text[MAX_TOKEN_TEXT_LEN+1];
    int vars[100];
    long exprloc;
} Token;

/* rule engine events */
typedef enum ruleEngineEvent {
	EXEC_RULE_BEGIN, /* execute a rule */
	EXEC_ACTION_BEGIN, /* execute an action */
	EXEC_MICRO_SERVICE_BEGIN, /* execute a microservice */
	EXEC_RULE_END, /* execute a rule */
	EXEC_ACTION_END, /* execute an action */
	EXEC_MICRO_SERVICE_END, /* execute a microservice */
	GOT_RULE, /* got a rule from rule index */
	APPLY_RULE_BEGIN, /* apply a rule */
	APPLY_RULE_END, /* apply a rule */
	APPLY_ALL_RULES_BEGIN, /* apply all rules */
	APPLY_ALL_RULES_END, /* apply all rules */
	EXEC_MY_RULE_BEGIN, /* execute user submitted rule */
	EXEC_MY_RULE_END /* execute user submitted rule */
} RuleEngineEvent;

typedef struct ruleEngineEventParam {
	int ruleIndex;
	char *actionName;
} RuleEngineEventParam;

Node *newNode(NodeType type, char* text, Label * exprloc, Region *r);
Node *newExprType(NodeType t, int degree, Node **subtrees, Region *r);
ExprType *newTVar(Region *r);
ExprType *newTVar2(int numDisjuncts, Node **disjuncts, Region *r);
ExprType *newCollType(ExprType *elemType, Region *r);
ExprType *newTupleType(int arity, ExprType **typeArgs, Region *r);
ExprType *newUnaryType(NodeType nodeType, ExprType *typeArg, Region *r);
ExprType *newFuncType(ExprType *paramType, ExprType *retType, Region *r);
ExprType *newFuncTypeVarArg(int arity, int vararg, ExprType **paramTypes, ExprType *elemType, Region *r);
ExprType *newConsType(int arity, char *cons, ExprType **paramTypes, Region *r);
ExprType *newTupleTypeVarArg(int arity, int vararg, ExprType **paramTypes, Region *r);
ExprType *newSimpType(NodeType t, Region *r);
ExprType *newErrorType(int errcode, Region *r);
ExprType *newIRODSType(char *name, Region *r);
ExprType *newFlexKind(int arity, ExprType **typeArgs, Region *r);
FunctionDesc *newFuncSymLink(char *fn , int nArgs, Region *r);
Node *newPartialApplication(Node *func, Node *arg, int nArgsLeft, Region *r);

/** Res functions */
Res* newRes(Region *r);
Res* newIntRes(Region *r, int n);
Res* newDoubleRes(Region *r, double a);
Res* newBoolRes(Region *r, int n);
Res* newErrorRes(Region *r, int errcode);
Res* newUnspecifiedRes(Region *r);
Res* newStringRes(Region *r, char *s);
Res* newPathRes(Region *r, char *s);
Res* newDatetimeRes(Region *r, long dt);
Res* newCollRes(int size, ExprType *elemType, Region *r);
Res* newUninterpretedRes(Region *r, char *typeName, void *ioStruct, bytesBuf_t *ioBuf);
Res* newTupleRes(int arity, Res **compTypes, Region *r);
msParam_t *newMsParam(char *typeName, void *ioStruct, bytesBuf_t *ioBuf, Region *r);

Env *newEnv(Hashtable *current, Env *previous, Env *lower, Region *r);
/* void deleteEnv(Env *env, int deleteCurrent); */
msParamArray_t *newMsParamArray();
void deleteMsParamArray(msParamArray_t *msParamArray);

TypingConstraint *newTypingConstraint(ExprType *a, ExprType *b, NodeType type, Node *node, Region *r);

FunctionDesc *newFunctionFD(char* type, SmsiFuncTypePtr func, Region *r);
FunctionDesc *newConstructorFD(char* type, Region *r);
FunctionDesc *newExternalFD(Node* type, Region *r);
FunctionDesc *newConstructorFD2(Node* type, Region *r);
FunctionDesc *newDeconstructorFD(char *type, int proj, Region *r);
FunctionDesc *newRuleIndexListFD(RuleIndexList *ruleIndexList, ExprType *, Region *r);

void setBase(Node *node, char *base, Region *r);
Node **setDegree(Node *node, int d, Region *r);
Node *createUnaryFunctionNode(char *fn, Node *a, Label * exprloc, Region *r);
Node *createBinaryFunctionNode(char *fn, Node *a, Node *b, Label * exprloc, Region *r);
Node *createFunctionNode(char *fn, Node **params, int paramsLen, Label * exprloc, Region *r);
Node *createActionsNode(Node **params, int paramsLen, Label * exprloc, Region *r);
Node *createTextNode(char *t, Label * exprloc, Region *r);
Node *createNumberNode(char *t, Label * exprloc, Region *r);
Node *createStringNode(char *t, Label * exprloc, Region *r);
Node *createErrorNode(char *error, Label * exprloc, Region *r);

RuleSet *newRuleSet(Region *r);
RuleDesc *newRuleDesc(RuleType rk, Node *n, int dynamictyping, Region *r);

#endif
