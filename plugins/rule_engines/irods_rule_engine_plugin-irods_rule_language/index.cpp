/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "index.hpp"
#include "rules.hpp"
#include "debug.hpp"
#include "configuration.hpp"
#define RE_ERROR(x) if(x) { goto error; }

#include <assert.h>

Hashtable *coreRuleFuncMapDefIndex = nullptr;
Hashtable *appRuleFuncMapDefIndex = nullptr;
Hashtable *microsTableIndex = nullptr;

void clearIndex( Hashtable **ruleIndex ) {
    if ( *ruleIndex != nullptr ) {
        deleteHashTable( *ruleIndex, free_const );
        *ruleIndex = nullptr;
    }
}
/**
 * returns 0 if out of memory
 */
int createRuleStructIndex( ruleStruct_t *inRuleStrct, Hashtable *ruleIndex ) {
    if ( ruleIndex == nullptr ) {
        return 0;
    }
    for ( int i = 0; i < inRuleStrct->MaxNumOfRules; i++ ) {
        char *key = inRuleStrct->action[i];
        int *value = ( int * )malloc( sizeof( int ) );
        *value = i;

        if ( 0 == insertIntoHashTable( ruleIndex, key, value ) ) {
            free( value );
            return 0;
        }
        free( value );
    }
    return 1;
}

int createCondIndex( Region *r ) {
    /* generate rule condition index */
    int i;
    for ( i = 0; i < ruleEngineConfig.coreFuncDescIndex->current->size; i++ ) {
        struct bucket *b = ruleEngineConfig.coreFuncDescIndex->current->buckets[i];

        struct bucket *resumingBucket = b;

        while ( resumingBucket != nullptr ) {

            FunctionDesc *fd = ( FunctionDesc * ) resumingBucket->value;
            resumingBucket = resumingBucket->next;
            if ( getNodeType( fd ) != N_FD_RULE_INDEX_LIST ) {
                continue;
            }
            RuleIndexList *ruleIndexList = FD_RULE_INDEX_LIST( fd );
#ifdef DEBUG_INDEX
            printf( "processing rule pack %s into condIndex\n", resumingBucket->key );
#endif


            RuleIndexListNode *currIndexNode = ruleIndexList->head;

            while ( currIndexNode != nullptr ) {
                Hashtable *processedStrs = newHashTable2( MAX_NUM_RULES * 2, r );
                RuleIndexListNode *startIndexNode = currIndexNode;
                RuleIndexListNode *finishIndexNode = nullptr;
                int groupCount = 0;
                Node *condExp = nullptr;
                Node *params = nullptr;

                while ( currIndexNode != nullptr ) {
                    RuleDesc *rd = getRuleDesc( currIndexNode->ruleIndex );
                    Node *ruleNode = rd->node;
                    if ( !(
                                rd->ruleType == RK_REL
                            ) ) {
                        finishIndexNode = currIndexNode;
                        currIndexNode = currIndexNode->next;
                        break;
                    }
                    Node *ruleCond = ruleNode->subtrees[1];
                    while ( getNodeType( ruleCond ) == N_TUPLE && ruleCond->degree == 1 ) {
                        ruleCond = ruleCond->subtrees[0];
                    }
                    if ( !(
                                getNodeType( ruleCond ) == N_APPLICATION &&
                                getNodeType( ruleCond->subtrees[0] ) == TK_TEXT &&
                                strcmp( ruleCond->subtrees[0]->text, "==" ) == 0 && /* comparison */
                                getNodeType( ruleCond->subtrees[1] ) == N_TUPLE &&
                                ruleCond->subtrees[1]->degree == 2 &&
                                getNodeType( ruleCond->subtrees[1]->subtrees[1] ) == TK_STRING /* with a string */
                            ) ) {
                        finishIndexNode = currIndexNode;
                        currIndexNode = currIndexNode->next;
                        break;
                    }
                    char *strVal = ruleCond->subtrees[1]->subtrees[1]->text;
                    if ( lookupFromHashTable( processedStrs, strVal ) != nullptr /* no repeated string */
                       ) {
                        finishIndexNode = currIndexNode;
                        break;
                    }
                    int i;
                    if ( condExp == nullptr ) {
                        condExp = ruleCond->subtrees[1]->subtrees[0];
                        params = ruleNode->subtrees[0]->subtrees[0];

                    }
                    else if ( RULE_NODE_NUM_PARAMS( ruleNode ) == params->degree ) {
                        Hashtable *varMapping = newHashTable2( 100, r );
                        for ( i = 0; i < params->degree; i++ ) {
                            updateInHashTable( varMapping, params->subtrees[i]->text, ruleNode->subtrees[0]->subtrees[0]->subtrees[i]->text );
                        }
                        if ( !eqExprNodeSyntacticVarMapping( condExp, ruleCond->subtrees[0], varMapping ) ) {
                            finishIndexNode = currIndexNode;
                            break;
                        }
                    }
                    else {
                        finishIndexNode = currIndexNode;
                        break;
                    }

                    insertIntoHashTable( processedStrs, strVal, strVal );
                    groupCount ++;

                    currIndexNode = currIndexNode ->next;
                }

                /* generate cond index if there are more than COND_INDEX_THRESHOLD rules in this group */
                if ( groupCount < COND_INDEX_THRESHOLD ) {
                    continue;
                }

#ifdef DEBUG_INDEX
                printf( "inserting rule group %s(%d) into ruleEngineConfig.condIndex\n", resumingBucket->key, groupCount );
#endif
                Hashtable *groupHashtable = newHashTable2( groupCount * 2, r );
                CondIndexVal *civ = newCondIndexVal( condExp, params, groupHashtable, r );

                RuleIndexListNode *instIndexNode = startIndexNode;
                while ( instIndexNode != finishIndexNode ) {
                    int ri = instIndexNode->ruleIndex;
                    Node *ruleNode = getRuleDesc( ri )->node;
                    removeNodeFromRuleIndexList( ruleIndexList, instIndexNode );
                    Node *ruleCond = ruleNode->subtrees[1];
                    while ( getNodeType( ruleCond ) == N_TUPLE && ruleCond->degree == 1 ) {
                        ruleCond = ruleCond->subtrees[0];
                    }
                    char *strVal = ruleCond->subtrees[1]->subtrees[1]->text;
#ifdef DEBUG_INDEX
                    printf( "inserting rule cond str %s, index %d into ruleEngineConfig.condIndex\n", strVal, ri );
#endif

                    insertIntoHashTable( groupHashtable, strVal, instIndexNode );
                    instIndexNode = instIndexNode->next;
                }
                insertIntoRuleIndexList( ruleIndexList, startIndexNode->prev, civ, r );
            }

        }

    }
    return 1;

}
void insertIntoRuleIndexList( RuleIndexList *rd, RuleIndexListNode *prev, CondIndexVal *civ, Region *r ) {
    if ( prev == nullptr ) {
        RuleIndexListNode *n = newRuleIndexListNode2( civ, prev, rd->head, r );
        rd->head = n;
        if ( rd->tail == nullptr ) {
            rd->tail = n;
        }
    }
    else {
        RuleIndexListNode *n = newRuleIndexListNode2( civ, prev, prev->next, r );
        if ( prev->next == nullptr ) {
            rd->tail = n;
        }
        prev->next = n;
    }
}
void removeNodeFromRuleIndexList2( RuleIndexList *rd, int i ) {
    RuleIndexListNode *n = rd->head;
    while ( n != nullptr && n->ruleIndex != i ) {
        n = n->next;
    }
    if ( n != nullptr ) {
        removeNodeFromRuleIndexList( rd, n );
    }
}
void removeNodeFromRuleIndexList( RuleIndexList *rd, RuleIndexListNode *n ) {
    if ( n == rd->head ) {
        rd->head = n->next;
    }
    if ( n == rd->tail ) {
        rd->tail = n->next;
    }
    if ( n->next != nullptr ) {
        n->next->prev = n->prev;
    }
    if ( n->prev != nullptr ) {
        n->prev->next = n->next;
    }
}
void appendRuleNodeToRuleIndexList( RuleIndexList *list, int i, Region *r ) {
    RuleIndexListNode *listNode = newRuleIndexListNode( i, list->tail, nullptr, r );
    list->tail ->next = listNode;
    list->tail = listNode;

}
void prependRuleNodeToRuleIndexList( RuleIndexList *list, int i, Region *r ) {
    RuleIndexListNode *listNode = newRuleIndexListNode( i, nullptr, list->head, r );
    listNode->next = list->head;
    list->head = listNode;

}
/**
 * returns 0 if out of memory
 */
int createRuleNodeIndex( RuleSet *inRuleSet, Hashtable *ruleIndex, int offset, Region *r ) {
    /* generate main index */
    int i;
    for ( i = 0; i < inRuleSet->len; i++ ) {
        RuleDesc *rd = inRuleSet->rules[i];
        Node *ruleNode = rd->node;
        if ( ruleNode == nullptr ) {
            continue;
        }
        RuleType ruleType = rd->ruleType;
        if ( ruleType == RK_REL || ruleType == RK_FUNC ) {
            char *key = ruleNode->subtrees[0]->text;
            FunctionDesc *fd = ( FunctionDesc * ) lookupFromHashTable( ruleIndex, key );
            if ( fd != nullptr ) {
                /*				printf("adding %s\n", key);*/
                if ( getNodeType( fd ) == N_FD_RULE_INDEX_LIST ) {
                    RuleIndexList *list = FD_RULE_INDEX_LIST( fd );
                    appendRuleNodeToRuleIndexList( list, i + offset, r );
                }
                else if ( getNodeType( fd ) == N_FD_EXTERNAL ) {
                    /* combine N_FD_EXTERNAL with N_FD_RULE_LIST */
                    if ( updateInHashTable( ruleIndex, key, newRuleIndexListFD( newRuleIndexList( key, i + offset, r ), fd->exprType, r ) ) == nullptr ) {
                        return 0;
                    }
                }
                else {
                    /* todo error handling */
                    return -1;
                }
            }
            else {
                /*				printf("inserting %s\n", key);*/
                if ( insertIntoHashTable( ruleIndex, key, newRuleIndexListFD( newRuleIndexList( key, i + offset, r ), nullptr, r ) ) == 0 ) {
                    return 0;
                }
            }
        }
    }

    return 1;
}
/**
 * returns 0 if out of memory
 */
int createFuncMapDefIndex( rulefmapdef_t *inFuncStrct, Hashtable **ruleIndex ) {
    clearIndex( ruleIndex );
    *ruleIndex = newHashTable( MAX_NUM_OF_DVARS * 2 );
    if ( *ruleIndex == nullptr ) {
        return 0;
    }
    for ( int i = 0; i < inFuncStrct->MaxNumOfFMaps; i++ ) {
        char *key = inFuncStrct->funcName[i];
        int *value = ( int * )malloc( sizeof( int ) );
        *value = i;

        if ( 0 == insertIntoHashTable( *ruleIndex, key, value ) ) {
            deleteHashTable( *ruleIndex, free_const );
            *ruleIndex = nullptr;
            free( value );
            return 0;
        }
        free( value );
    }
    return 1;
}

/* find the ith RuleIndexListNode */
int findNextRuleFromIndex( Env *ruleIndex, const char *action, int i, RuleIndexListNode **node ) {
    int k = i;
    if ( ruleIndex != nullptr ) {
        FunctionDesc *fd = ( FunctionDesc * )lookupFromHashTable( ruleIndex->current, action );
        if ( fd != nullptr ) {
            if ( getNodeType( fd ) != N_FD_RULE_INDEX_LIST ) {
                return NO_MORE_RULES_ERR;
            }
            RuleIndexList *l = FD_RULE_INDEX_LIST( fd );
            RuleIndexListNode *b = l->head;
            while ( k != 0 ) {
                if ( b != nullptr ) {
                    b = b->next;
                    k--;
                }
                else {
                    break;
                }
            }
            if ( b != nullptr ) {
                *node = b;
                return 0;
            }
        }
        return findNextRuleFromIndex( ruleIndex->previous, action, k, node );
    }

    return NO_MORE_RULES_ERR;
}
/**
 * adapted from original code
 */
int findNextRule2( const char *action,  int i, RuleIndexListNode **node ) {
    if ( isComponentInitialized( ruleEngineConfig.extFuncDescIndexStatus ) ) {
        int ii = findNextRuleFromIndex( ruleEngineConfig.extFuncDescIndex, action, i, node );
        if ( ii != NO_MORE_RULES_ERR ) {
            return 0;
        }
        else {
            return NO_MORE_RULES_ERR;
        }
    }
    else {
        return NO_MORE_RULES_ERR;
    }
}

int mapExternalFuncToInternalProc2( char *funcName ) {
    int *i;

    if ( appRuleFuncMapDefIndex != nullptr && ( i = ( int * )lookupFromHashTable( appRuleFuncMapDefIndex, funcName ) ) != nullptr ) {
        strcpy( funcName, appRuleFuncMapDef.func2CMap[*i] );
        return 1;
    }
    if ( coreRuleFuncMapDefIndex != nullptr && ( i = ( int * )lookupFromHashTable( coreRuleFuncMapDefIndex, funcName ) ) != nullptr ) {
        strcpy( funcName, coreRuleFuncMapDef.func2CMap[*i] );
        return 1;
    }
    return 0;
}
int actionTableLookUp2( char *action ) {
    int *i;

    if ( ( i = ( int * )lookupFromHashTable( microsTableIndex, action ) ) != nullptr ) {
        return *i;
    }

    return UNMATCHED_ACTION_ERR;
}
void deleteCondIndexVal( CondIndexVal *h ) {
    deleteHashTable( h->valIndex, nop );
}

char *convertRuleNameArityToKey( char *ruleName, int arity ) {
    // assume that arity < 100
    char *key = ( char * )malloc( strlen( ruleName ) + 3 );
    sprintf( key, "%02d%s", arity, ruleName );
    return key;
}

RuleIndexList *newRuleIndexList( char *ruleName, int ruleIndex, Region *r ) {
    RuleIndexList *list = ( RuleIndexList * )region_alloc( r, sizeof( RuleIndexList ) );
    list->ruleName = cpStringExt( ruleName, r );
    list->head = list->tail = newRuleIndexListNode( ruleIndex, nullptr, nullptr, r );
    return list;
}

RuleIndexListNode *newRuleIndexListNode( int ruleIndex, RuleIndexListNode *prev, RuleIndexListNode *next, Region *r ) {
    RuleIndexListNode *node = ( RuleIndexListNode * )region_alloc( r, sizeof( RuleIndexListNode ) );
    memset( node, 0, sizeof( RuleIndexListNode ) );
    node->ruleIndex = ruleIndex;
    node->secondaryIndex = 0;
    node->prev = prev;
    node->next = next;
    return node;

}

RuleIndexListNode *newRuleIndexListNode2( CondIndexVal *civ, RuleIndexListNode *prev, RuleIndexListNode *next, Region *r ) {
    RuleIndexListNode *node = ( RuleIndexListNode * )region_alloc( r, sizeof( RuleIndexListNode ) );
    memset( node, 0, sizeof( RuleIndexListNode ) );
    node->condIndex = civ;
    node->secondaryIndex = 1;
    node->prev = prev;
    node->next = next;
    return node;

}
CondIndexVal *newCondIndexVal( Node *condExp, Node *params, Hashtable *groupHashtable, Region *r ) {
    CondIndexVal *civ = ( CondIndexVal * )region_alloc( r, sizeof( CondIndexVal ) );
    civ->condExp = condExp;
    civ->params = params;
    civ->valIndex = groupHashtable;
    return civ;
}
