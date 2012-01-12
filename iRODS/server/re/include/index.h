/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef INDEX_H
#define INDEX_H
#include "debug.h"
#include "hashtable.h"
#include "rules.h"
#ifndef DEBUG
#include "reGlobalsExtern.h"
#endif

#define COND_INDEX_THRESHOLD 2

char *convertRuleNameArityToKey(char *ruleName, int arity);
RuleIndexList *newRuleIndexList(char *ruleName, int ruleIndex, Region *r);
RuleIndexListNode *newRuleIndexListNode(int ruleIndex, RuleIndexListNode *prev, RuleIndexListNode *next, Region *r);
RuleIndexListNode *newRuleIndexListNode2(CondIndexVal* civ, RuleIndexListNode *prev, RuleIndexListNode *next, Region *r);
CondIndexVal *newCondIndexVal(Node *condExp, Node *params, Hashtable *groupHashtable, Region *r);

extern Hashtable *ruleIndex;
extern Hashtable *coreRuleFuncMapDefIndex;
extern Hashtable *appRuleFuncMapDefIndex;
extern Hashtable *microsTableIndex;
/* this is an index of indexed rules */
/* indexed rules are rules such that */
/* 1. all rules has the same rule name */
/* 2. all rules has an empty param list */
/* 3. no other rule has the same rule name */
/* 4. the rule conditions are all of the form exp = "string", where exp is the same and "string" is distinct for all rules */
/* when a subset of rules are indexed rules, the condIndex */
extern Hashtable *condIndex; /* char * -> CondIndexVal * */

void clearIndex(Hashtable **ruleIndex);

int createRuleStructIndex(ruleStruct_t *inRuleStrct, Hashtable *ruleIndex);
int createRuleNodeIndex(RuleSet *inRuleSet, Hashtable *ruleIndex, int offset, Region *r);
int createCoreAppExtRuleNodeIndex();
int createCondIndex(Region *r);
int createFuncMapDefIndex(rulefmapdef_t *inFuncStrct1, Hashtable **ruleIndex);
/* int clearRuleSet(RuleSet *inRuleSet); */

int mapExternalFuncToInternalProc2(char *funcName);
int findNextRuleFromIndex(Env *ruleIndex, char *action, int i, RuleIndexListNode **node);
int findNextRule2(char *action,  int i, RuleIndexListNode **node);
int actionTableLookUp2(char *action);
int createMacorsIndex();
void deleteCondIndexVal(CondIndexVal *h);
void insertIntoRuleIndexList(RuleIndexList *rd, RuleIndexListNode *prev, CondIndexVal *civ, Region *r);
void removeNodeFromRuleIndexList2(RuleIndexList *rd, int i);
void removeNodeFromRuleIndexList(RuleIndexList *rd, RuleIndexListNode *node);
void appendRuleNodeToRuleIndexList(RuleIndexList *rd, int i, Region *r);
void prependRuleNodeToRuleIndexList(RuleIndexList *rd, int i, Region *r);

#endif
