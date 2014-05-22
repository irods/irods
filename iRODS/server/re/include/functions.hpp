/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef FUNCTIONS_HPP
#define	FUNCTIONS_HPP

#include "hashtable.hpp"
#include "region.hpp"
#include "parser.hpp"
#include "debug.hpp"

#ifdef DEBUG
#include "re.hpp"
#endif

typedef struct reIterableData {
    char *varName;
    Res *res;
    void *itrSpecData;
    Res *errorRes;
    Node **subtrees;
    Node *node;
    ruleExecInfo_t *rei;
    int reiSaveFlag;
    Env *env;
    rError_t* errmsg;
} ReIterableData;

typedef enum reIterableType {
    RE_ITERABLE_COMMA_STRING,
    RE_ITERABLE_STRING_ARRAY,
    RE_ITERABLE_INT_ARRAY,
    RE_ITERABLE_LIST,
    RE_ITERABLE_COLLECTION,
    RE_ITERABLE_GEN_QUERY,
    RE_ITERABLE_GEN_QUERY_OUT,
    RE_ITERABLE_KEY_VALUE_PAIRS,
    RE_NOT_ITERABLE,
} ReIterableType;

typedef struct reIterable {
    void ( *init )( ReIterableData *itrSpecData, Region *r );
    int ( *hasNext )( ReIterableData *itrSpecData, Region *r );
    Res *( *next )( ReIterableData *itrSpecData, Region *r );
    void ( *finalize )( ReIterableData *itrSpecData, Region *r );
} ReIterable;

typedef struct reIterableTableRow {
    ReIterableType nodeType;
    ReIterable reIterable;
} ReIterableTableRow;

ReIterableData *newReIterableData(
    char *varName,
    Res *res,
    Node **subtrees,
    Node *node,
    ruleExecInfo_t *rei,
    int reiSaveFlag,
    Env *env,
    rError_t* errmsg
);
void deleteReIterableData( ReIterableData *itrData );

void getSystemFunctions( Hashtable *ft, Region *r );

Res* eval( char *expr, Env *env, ruleExecInfo_t *rei, int saveREI, rError_t *errmsg, Region *r );

int getParamIOType( char *iotypes, int index );

FunctionDesc *getFuncDescFromChain( int n, FunctionDesc *fDesc );
Node *construct( char *fn, Node **args, int argc, Node* constype, Region *r );
Node *deconstruct( char *fn, Node **args, int argc, int proj, rError_t*errmsg, Region *r );
char* matchWholeString( char *buf );
char *wildCardToRegex( char *buf );

#endif	/* FUNCTIONS_H */

