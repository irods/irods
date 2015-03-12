/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "utils.hpp"
#include "restructs.hpp"
#include "parser.hpp"

/**
 * create a new node n
 */
Node *newNode( NodeType type, const char* text, Label * eloc, Region *r ) {
    Node *node = ( Node * )region_alloc( r, sizeof( Node ) );
    if ( node == NULL ) {
        return NULL;
    }
    memset( node, 0, sizeof( Node ) );
    setNodeType( node, type );
    if ( text != NULL ) {
        node->text = ( char * )region_alloc( r, ( strlen( text ) + 1 ) * sizeof( char ) );
        strcpy( node->text, text );
    }
    else {
        node->text = NULL;
    }
    NODE_EXPR_POS( node ) = eloc == NULL ? 0 : eloc->exprloc;
    setIOType( node, IO_TYPE_INPUT );
    if ( eloc != NULL ) {
        setBase( node, eloc->base, r );
    }
    else {
        setBase( node, "", r );
    }
    return node;
}
Node **allocSubtrees( Region *r, int size ) {
    return ( Node** )region_alloc( r, sizeof( Node* ) * size );
}


Node *newExprType( NodeType type, int degree, Node **subtrees, Region *r ) {
    ExprType *t = ( ExprType * )region_alloc( r, sizeof( ExprType ) );
    memset( t, 0, sizeof( ExprType ) );
    t->subtrees = subtrees;
    t->degree = degree;
    setNodeType( t, type );
    t->option |= OPTION_TYPED;
    setIOType( t, IO_TYPE_INPUT );
    return t;

}

ExprType *newTVar2( int numDisjuncts, Node **disjuncts, Region *r ) {
    ExprType *t = newExprType( T_VAR, 0, NULL, r );
    T_VAR_ID( t ) = newTVarId();
    T_VAR_NUM_DISJUNCTS( t ) = numDisjuncts;
    T_VAR_DISJUNCTS( t ) = numDisjuncts == 0 ? NULL : ( Node ** )region_alloc( r, sizeof( Node * ) * numDisjuncts );
    if ( numDisjuncts != 0 ) {
        memcpy( T_VAR_DISJUNCTS( t ), disjuncts, numDisjuncts * sizeof( Node * ) );
    }
    return t;
}

ExprType *newTVar( Region *r ) {
    ExprType *t = newExprType( T_VAR, 0, NULL, r );
    T_VAR_ID( t ) = newTVarId();
    T_VAR_NUM_DISJUNCTS( t ) = 0;
    T_VAR_DISJUNCTS( t ) = NULL;
    return t;
}

ExprType *newSimpType( NodeType type, Region *r ) {
    return newExprType( type, 0, NULL, r );
}
ExprType *newErrorType( int errcode, Region *r ) {
    Res *res = newExprType( T_ERROR, 0, NULL, r );
    RES_ERR_CODE( res ) = errcode;
    return res;

}
ExprType *newFuncType( ExprType *paramType, ExprType* retType, Region *r ) {
    ExprType **typeArgs = ( ExprType ** )region_alloc( r, sizeof( ExprType * ) * 2 );
    typeArgs[0] = paramType;
    typeArgs[1] = retType;
    return newConsType( 2, cpStringExt( FUNC, r ), typeArgs, r );
}
ExprType *newFuncTypeVarArg( int arity, int vararg, ExprType **paramTypes, ExprType* retType, Region *r ) {
    return newFuncType( newTupleTypeVarArg( arity, vararg, paramTypes, r ), retType, r );
}
ExprType *newConsType( int arity, char *cons, ExprType **paramTypes, Region *r ) {
    ExprType *t = newExprType( T_CONS, arity, paramTypes, r );
    T_CONS_TYPE_NAME( t ) = cpString( cons, r );
    return t;
}
ExprType *newTupleTypeVarArg( int arity, int vararg, ExprType **paramTypes, Region *r ) {
    ExprType *t = newExprType( T_TUPLE, arity, paramTypes, r );
    setVararg( t, vararg );
    return t;
}

ExprType *newIRODSType( const char *name, Region *r ) {
    ExprType *t = newExprType( T_IRODS, 0, NULL, r );
    t->text = ( char * )region_alloc( r, ( strlen( name ) + 1 ) * sizeof( char ) );
    strcpy( t->text, name );
    return t;
}
ExprType *newCollType( ExprType *elemType, Region *r ) {
    ExprType **typeArgs = ( ExprType** ) region_alloc( r, sizeof( ExprType* ) );
    typeArgs[0] = elemType;
    return newConsType( 1, cpStringExt( LIST, r ), typeArgs, r );
}

ExprType *newTupleType( int arity, ExprType **typeArgs, Region *r ) {
    return newExprType( T_TUPLE, arity, typeArgs, r );
}
ExprType *newUnaryType( NodeType nodeType, ExprType *typeArg, Region *r ) {
    ExprType **typeArgs = ( ExprType** ) region_alloc( r, sizeof( ExprType* ) );
    typeArgs[0] = typeArg;
    return newExprType( nodeType, 1, typeArgs, r );
}
/* ExprType *newFlexKind(int arity, ExprType **typeArgs, Region *r) {
	return newExprType(K_FLEX, arity, typeArgs, r);
} */

FunctionDesc *newFuncSymLink( char *fn , int nArgs, Region *r ) {
    Res *desc = newRes( r );
    setNodeType( desc, N_SYM_LINK );
    desc->text = cpStringExt( fn , r );
    RES_FUNC_N_ARGS( desc ) = nArgs;
    desc->exprType = newSimpType( T_DYNAMIC, r );
    return desc;
}

Node *newPartialApplication( Node *func, Node *arg, int nArgsLeft, Region *r ) {
    Res *res1 = newRes( r );
    setNodeType( res1, N_PARTIAL_APPLICATION );
    RES_FUNC_N_ARGS( res1 ) = nArgsLeft;
    res1->degree = 2;
    res1->subtrees = ( Res ** )region_alloc( r, sizeof( Res * ) * 2 );
    res1->subtrees[0] = func;
    res1->subtrees[1] = arg;
    return res1;
}

Node *newTupleRes( int arity, Res **comps, Region *r ) {
    Res *res1 = newRes( r );
    setNodeType( res1, N_TUPLE );
    res1->subtrees = comps;
    res1->degree = arity;
    ExprType **compTypes = ( ExprType ** )region_alloc( r, sizeof( ExprType * ) * arity );
    int i;
    for ( i = 0; i < arity; i++ ) {
        compTypes[i] = comps[i]->exprType;
    }
    res1->exprType = newTupleType( arity, compTypes, r );
    return res1;
}
Res* newCollRes( int size, ExprType *elemType, Region *r ) {
    Res *res1 = newRes( r );
    res1->exprType = newCollType( elemType, r );
    res1->degree = size;
    res1->subtrees = ( Res ** )region_alloc( r, sizeof( Res * ) * size );
    return res1;
}
/* used in cpRes only */
Res* newCollRes2( int size, Region *r ) {
    Res *res1 = newRes( r );
    res1->exprType = NULL;
    res1->degree = size;
    res1->subtrees = ( Res ** )region_alloc( r, sizeof( Res * ) * size );
    return res1;
}
Res* newRes( Region *r ) {
    Res *res1 = ( Res * ) region_alloc( r, sizeof( Res ) );
    memset( res1, 0, sizeof( Res ) );
    setNodeType( res1, N_VAL );
    setIOType( res1, IO_TYPE_INPUT );
    return res1;
}
Res* newUninterpretedRes( Region *r, char *typeName, void *ioStruct, bytesBuf_t *ioBuf ) {
    Res *res1 = newRes( r );
    res1->exprType = newIRODSType( typeName, r );
    res1->param = newMsParam( typeName, ioStruct, ioBuf, r );
    return res1;
}
msParam_t *newMsParam( const char *typeName, void *ioStruct, bytesBuf_t *ioBuf, Region *r ) {
    msParam_t *param = ( msParam_t * ) region_alloc( r, sizeof( msParam_t ) );
    memset( param, 0, sizeof( msParam_t ) );
    param->type = typeName ? strdup( typeName ) : NULL;
    param->inOutStruct = ioStruct;
    param->inpOutBuf = ioBuf;
    return param;

}
Res* newIntRes( Region *r, int n ) {
    Res *res1 = newRes( r );
    res1->exprType = newSimpType( T_INT, r );
    RES_INT_VAL_LVAL( res1 ) = n;
    return res1;
}
Res* newDoubleRes( Region *r, double a ) {
    Res *res1 = newRes( r );
    res1->exprType = newSimpType( T_DOUBLE, r );
    RES_DOUBLE_VAL_LVAL( res1 ) = a;
    return res1;
}
Res* newBoolRes( Region *r, int n ) {
    Res *res1 = newRes( r );
    res1->exprType = newSimpType( T_BOOL, r );
    RES_BOOL_VAL_LVAL( res1 ) = n;
    return res1;
}
Res* newStringBasedRes( Region *r, char *s ) {
    Res *res1 = newRes( r );
    RES_STRING_STR_LEN( res1 ) = strlen( s );
    int size = ( RES_STRING_STR_LEN( res1 ) + 1 ) * sizeof( char );
    res1->text = ( char * )region_alloc( r, size );
    memcpy( res1->text, s, size );
    return res1;
}
Res *newStringRes( Region *r, char *s ) {
    Res *res = newStringBasedRes( r, s );
    res->exprType = newSimpType( T_STRING, r );
    return res;

}
Res *newPathRes( Region *r, char *s ) {
    Res *res = newStringBasedRes( r, s );
    res->exprType = newSimpType( T_PATH, r );
    return res;

}
Res* newUnspecifiedRes( Region *r ) {
    Res *res1 = newRes( r );
    res1->exprType = newSimpType( T_UNSPECED, r );
    res1->text = cpStringExt( "", r );
    return res1;
}
Res* newDatetimeRes( Region *r, long dt ) {
    Res *res1 = newRes( r );
    res1->exprType = newSimpType( T_DATETIME, r );
    RES_TIME_VAL( res1 ) = dt;
    return res1;
}
Res* newErrorRes( Region *r, int errcode ) {
    Res *res1 = newRes( r );
    setNodeType( res1, N_ERROR );
    RES_ERR_CODE( res1 ) = errcode;
    return res1;
}

msParamArray_t *newMsParamArray() {
    msParamArray_t *mpa = ( msParamArray_t * )malloc( sizeof( msParamArray_t ) );
    mpa->len = 0;
    mpa->msParam = NULL;
    mpa->oprType = 0;
    return mpa;
}

void deleteMsParamArray( msParamArray_t *msParamArray ) {
    clearMsParamArray( msParamArray, 0 ); /* do not delete inOutStruct because global varaibles of iRODS type may share it */
    /* to do write a function that delete inOutStruct of msParamArray if it is not shared */
    free( msParamArray );

}

Env *newEnv( Hashtable *current, Env *previous, Env *callerEnv, Region *r ) {
    Env *env = ( Env * )region_alloc( r, sizeof( Env ) );
    env->current = current;
    env->previous = previous;
    env->lower = callerEnv;
    return env;
}

/*void deleteEnv(Env *env, int deleteCurrent) {
    if(deleteCurrent>=1) {
        deleteHashTable(env->current, nop);
    }
    if(deleteCurrent==2) {
        if(env->previous!=NULL && env->previous->previous!=NULL) {
            deleteEnv(env->previous, deleteCurrent);
        }
    }
    if(deleteCurrent>=3) {
        if(env->previous!=NULL) {
            deleteEnv(env->previous, deleteCurrent);

        }
    }
    free(env);
}*/

TypingConstraint *newTypingConstraint( ExprType *a, ExprType *b, NodeType type, Node *node, Region *r ) {
    TypingConstraint *tc = ( TypingConstraint * )region_alloc( r, sizeof( TypingConstraint ) );
    memset( tc, 0, sizeof( TypingConstraint ) );
    tc->subtrees = ( Node ** )region_alloc( r, sizeof( Node * ) * 4 );
    TC_A( tc ) = a;
    TC_B( tc ) = b;
    setNodeType( tc, type );
    TC_NODE( tc ) = node;
    TC_NEXT( tc ) = NULL;
    tc->degree = 4;
    return tc;
}




FunctionDesc *newFunctionFD( char *type, SmsiFuncTypePtr func, Region *r ) {
    FunctionDesc *desc = ( FunctionDesc * ) region_alloc( r, sizeof( FunctionDesc ) );
    memset( desc, 0, sizeof( FunctionDesc ) );
    FD_SMSI_FUNC_PTR_LVAL( desc ) = func;
    desc->exprType = type == NULL ? NULL : parseFuncTypeFromString( type, r );
    setNodeType( desc, N_FD_FUNCTION );
    return desc;
}
FunctionDesc *newConstructorFD( char *type, Region *r ) {
    return newConstructorFD2( parseFuncTypeFromString( type, r ), r );
}

FunctionDesc *newConstructorFD2( Node *type, Region *r ) {
    FunctionDesc *desc = ( FunctionDesc * ) region_alloc( r, sizeof( FunctionDesc ) );
    memset( desc, 0, sizeof( FunctionDesc ) );
    desc->exprType = type;
    setNodeType( desc, N_FD_CONSTRUCTOR );
    return desc;
}
FunctionDesc *newExternalFD( Node *type, Region *r ) {
    FunctionDesc *desc = ( FunctionDesc * ) region_alloc( r, sizeof( FunctionDesc ) );
    memset( desc, 0, sizeof( FunctionDesc ) );
    desc->exprType = type;
    setNodeType( desc, N_FD_EXTERNAL );
    return desc;
}
FunctionDesc *newDeconstructorFD( char *type, int proj, Region *r ) {
    FunctionDesc *desc = ( FunctionDesc * ) region_alloc( r, sizeof( FunctionDesc ) );
    memset( desc, 0, sizeof( FunctionDesc ) );
    desc->exprType = type == NULL ? NULL : parseFuncTypeFromString( type, r );
    setNodeType( desc, N_FD_DECONSTRUCTOR );
    FD_PROJ( desc ) = proj;
    return desc;
}
FunctionDesc *newRuleIndexListFD( RuleIndexList *ruleIndexList, ExprType *type, Region *r ) {
    FunctionDesc *desc = ( FunctionDesc * ) region_alloc( r, sizeof( FunctionDesc ) );
    memset( desc, 0, sizeof( FunctionDesc ) );
    FD_RULE_INDEX_LIST_LVAL( desc ) = ruleIndexList;
    desc->exprType = type;
    setNodeType( desc, N_FD_RULE_INDEX_LIST );
    return desc;
}

RuleDesc *newRuleDesc( RuleType rk, Node *n, int dynamictyping, Region *r ) {
    RuleDesc *rd = ( RuleDesc * ) region_alloc( r, sizeof( RuleDesc ) );
    memset( rd, 0, sizeof( RuleDesc ) );
    rd->id = -1;
    rd->node = n;
    rd->type = NULL;
    rd->ruleType = rk;
    rd->dynamictyping = dynamictyping;
    return rd;
}

RuleSet *newRuleSet( Region *r ) {
    RuleSet *rs = ( RuleSet * ) region_alloc( r, sizeof( RuleSet ) );
    memset( rs, 0, sizeof( RuleSet ) );
    rs->len = 0;
    return rs;
}

void setBase( Node *node, char *base, Region *r ) {
    node->base = ( char * )region_alloc( r, sizeof( char ) * ( strlen( base ) + 1 ) );
    strcpy( node->base, base );

}
/**
 * set the degree of a node and allocate subtrees array
 */
Node **setDegree( Node *node, int d, Region *r ) {
    node->degree = d;
    node->subtrees = ( Node ** )region_alloc( r, d * sizeof( Node * ) );
    if ( node->subtrees == NULL ) {
        return NULL;
    }
    return node->subtrees;
}
/*
Node *dupNode(Node *node, Region* r) {
    Node *dup = newNode(node->type, node->text, node->expr, r);
    Node **subdup = setDegree(dup, node->degree, r);
    int i;
    for(i = 0;i<node->degree;i++) {
        subdup[i] = dupNode(node->subtrees[i],r);
    }
    return dup;
}*/
Node *createUnaryFunctionNode( char *fn, Node *a, Label * expr, Region *r ) {
    Node *node = newNode( N_APPLICATION, fn, expr, r );
    if ( node == NULL ) {
        return NULL;
    }
    setDegree( node, 1, r );
    node->subtrees[0] = a;
    return node;
}
Node *createBinaryFunctionNode( char *fn, Node *a, Node *b, Label * expr, Region *r ) {
    Node *node = newNode( N_APPLICATION, fn, expr, r );
    if ( node == NULL ) {
        return NULL;
    }
    setDegree( node, 2, r );
    node->subtrees[0] = a;
    node->subtrees[1] = b;
    return node;
}
Node *createFunctionNode( const char *fn, Node **params, int paramsLen, Label * exprloc, Region *r ) {
    Node *node = newNode( N_APPLICATION, fn, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    Node *func = newNode( TK_TEXT, fn, exprloc, r );
    Node *param = newNode( N_TUPLE, APPLICATION, exprloc, r );
    setDegree( param, paramsLen, r );
    memcpy( param->subtrees, params, paramsLen * sizeof( Node * ) );
    setDegree( node, 2, r );
    node->subtrees[0] = func;
    node->subtrees[1] = param;
    return node;
}
Node *createActionsNode( Node **params, int paramsLen, Label * exprloc, Region *r ) {
    Node *node = newNode( N_ACTIONS, "ACTIONS", exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    setDegree( node, paramsLen, r );
    memcpy( node->subtrees, params, paramsLen * sizeof( Node * ) );
    return node;
}
Node *createTextNode( char *t, Label * exprloc, Region *r ) {
    Node *node = newNode( TK_TEXT, t, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    return node;
}
Node *createIntNode( char *t, Label * exprloc, Region *r ) {
    Node *node = newNode( TK_INT, t, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    return node;
}
Node *createDoubleNode( char *t, Label * exprloc, Region *r ) {
    Node *node = newNode( TK_DOUBLE, t, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    return node;
}
Node *createStringNode( char *t, Label * exprloc, Region *r ) {
    Node *node = newNode( TK_STRING, t, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    return node;
}
Node *createErrorNode( char *error, Label * exprloc, Region *r ) {
    Node *node = newNode( N_ERROR, error, exprloc, r );
    if ( node == NULL ) {
        return NULL;
    }
    return node;
}

