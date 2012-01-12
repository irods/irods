/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef UTILS_H
#define UTILS_H
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#ifndef DEBUG
#include "objInfo.h"
#include "reHelpers1.h"
#include "rodsType.h"
#endif

#include "restructs.h"
#include "region.h"
#include "hashtable.h"

#define CASCASE_NON_ZERO(x) {int ret = x; if(ret != 0) { return ret;} }
#define CASCADE_NULL(x) if((x)==NULL) return NULL;
#define CASCADE_N_ERROR(x) {Res *ret = x; if(getNodeType(ret) == N_ERROR) { \
        return ret; \
    }}

#define IS_TVAR_NAME(x) ((x)[0] == '?')

int newTVarId();

char* getTVarName(int vid, char name[128]);
char* getTVarNameRegion(int vid, Region *r);
char* getTVarNameRegionFromExprType(ExprType *tvar, Region *r);

ExprType *dupType(ExprType *ty, Region *r);
int typeEqSyntatic(ExprType *a, ExprType *b);

Node **allocSubtrees(Region *r, int size);
void setStringPair(ExprType *ty, Region *r);
void replace(Hashtable *varTypes, int a, ExprType *b);
void replaceCons(ExprType *consType, int a, ExprType *b);
ExprType* dereference(ExprType *type, Hashtable *tt, Region *r);
ExprType *instantiate(ExprType *type, Hashtable *type_table, int replaceFreeVars, Region *r);


ExprType *dupTypeAux(ExprType *ty, Region *r, Hashtable *varTable);

#define cpRes(p, r) regionRegionCpNode(p, r)
#define cpType(p, r) regionRegionCpNode(p, r)
#define cpRes2(p, oldr, r) regionRegion2CpNode(p, oldr, r)
#define cpType2(p, oldr, r) regionRegion2CpNode(p, oldr, r)

/*Res *cpRes(Res *res, Region *r);*/
/*ExprType *cpType(ExprType *type, Region *r); */
char *cpString(char *res, Region *r);
char *cpStringExt(char* str, Region* r);
void cpHashtable(Hashtable *env, Region *r);
void cpEnv(Env *env, Region *r);
/*Res *cpRes2(Res *res, Region *oldr, Region *newr);*/
/*ExprType *cpType2(ExprType *type, Region *oldr, Region *newr);*/
char *cpString2(char *res, Region *oldr, Region *newr);
void cpHashtable2(Hashtable *env, Region *oldr, Region *newr);
void cpEnv2(Env *env, Region *oldr, Region *newr);

Res *setVariableValue(char *varName, Res *val, ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r);
int occursIn(ExprType *var, ExprType *type);
ExprType* unifyWith(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r);
ExprType* unifyNonTvars(ExprType *type, ExprType *expected, Hashtable *varTypes, Region *r);
ExprType* unifyTVarL(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r);
ExprType* unifyTVarR(ExprType *type, ExprType* expected, Hashtable *varTypes, Region *r);

void printType(ExprType *type, Hashtable *var_types);
char *typeToString(ExprType *type, Hashtable *var_types, char *buf, int bufsize);
void typingConstraintsToString(List *typingConstraints, Hashtable *var_types, char *buf, int bufsize);

void *lookupFromEnv(Env *env, char *key);
void updateInEnv(Env *env, char *varname, Res *res);
void freeEnvUninterpretedStructs(Env *e);
Env* globalEnv(Env *env);

void listAppend(List *list, void *value, Region *r);
void listAppendToNode(List *list, ListNode *node, void *value, Region *r);
void listRemove(List *list, ListNode *node);
void listAppendNoRegion(List *list, void *value);
void listRemoveNoRegion(List *list, ListNode *node);

int appendToByteBufNew(bytesBuf_t *bytesBuf, char *str);
void logErrMsg(rError_t *errmsg, rError_t *system);
char *errMsgToString(rError_t *errmsg, char *buf, int buflen);

int isPattern(Node *pattern);
int isRecursive(Node *rule);
int invokedIn(char *fn, Node *expr);
Node *lookupAVUFromMetadata(Node *metadata, char *a);

int isRuleGenSyntax(char *expr);

#define INC_MOD(x, m) (x) = (((x) == ((m) - 1)) ? 0 : ((x) + 1))
#define DEC_MOD(x, m) (x) = (((x) == 0) ? ((m) - 1) : ((x) - 1))
#define FLOOR_MOD(x, m) ((x) % (m) == 0? (x) : (x) / (m) * (m))
#define CEILING_MOD(x, m) ((x) % (m) == 0? (x) : (x) / (m) * (m) + (m))

/*#define INC_MOD(x, m) (x) = (x+1)% (m)*/
/*#define DEC_MOD(x, m) (x) = (x+(m)-1) % (m)*/
#define CONCAT2(a,b) a##b
#define CONCAT(a,b) CONCAT2(a,b)

#define KEY_PROTO
#include "key.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

void keyBuf(unsigned char *buf, int size, char *keyBuf);

#undef KEY_PROTO

#include "region.to.region.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "to.region.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "to.memory.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

#include "region.to.region2.proto.h"
#include "proto.h"
#include "restruct.templates.h"
#include "end.instance.h"

/*#include "region.check.proto.h"
#include "restruct.templates.h"
#include "end.instance.h"*/

/** debugging functions */
int writeToTmp(char *fileName, char *text);
void printMsParamArray(msParamArray_t *msParamArray, char *buf2);
void printHashtable(Hashtable *env, char* buf2);
void printVarTypeEnvToStdOut(Hashtable *env);


#endif
