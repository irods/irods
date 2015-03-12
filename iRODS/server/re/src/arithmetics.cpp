/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include "reFuncDefs.hpp"
#include "utils.hpp"
#include "restructs.hpp"
#include "parser.hpp"
#include "arithmetics.hpp"
#include "datetime.hpp"
#include "index.hpp"
#include "rules.hpp"
#include "functions.hpp"
#include "configuration.hpp"
#include "reVariableMap.gen.hpp"
#include "reVariableMap.hpp"
#include "debug.hpp"

//    #include "irods_ms_plugin.hpp"
//    extern irods::ms_table MicrosTable;
//    extern int NumOfAction;

#define RE_ERROR(x, y) if(x) {if((y)!=NULL){(y)->type.t=RE_ERROR;*errnode=node;}return;}
#define OUTOFMEMORY(x, res) if(x) {(res)->value.e = OUT_OF_MEMORY;TYPE(res) = RE_ERROR;return;}

#define RE_ERROR2(x,y) if(x) {localErrorMsg=(y);goto error;}
extern int GlobalREDebugFlag;
extern int GlobalREAuditFlag;

/* utilities */
int initializeEnv( Node *params, Res *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, Hashtable *env ) {


    Node** args2 = params->subtrees;
    int i;
    for ( i = 0; i < argc ; i++ ) {
        insertIntoHashTable( env, args2[i]->text, args[i] );
    }
    return 0;
}

char* getVariableName( Node *node ) {
    return node->subtrees[0]->text;
}


Res* evaluateExpression3( Node *expr, int applyAll, int force, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r ) {
    /*
        printTree(expr, 0);
    */
    char errbuf[ERR_MSG_LEN];
    Res *res = newRes( r ), *funcRes = NULL, *argRes = NULL;
    FunctionDesc *fd = NULL;
    int i;
    Res **tupleComps = NULL;
    /* Only input parameters are evaluated here;
     * The original AST node is needed for parameters of other IO types in evaluateFunction3.
     */
    if ( force || getIOType( expr ) == IO_TYPE_INPUT ) {
        switch ( getNodeType( expr ) ) {
        case TK_BOOL:
            res->exprType = newSimpType( T_BOOL, r );
            RES_BOOL_VAL_LVAL( res ) = strcmp( expr->text, "true" ) == 0 ? 1 : 0;
            break;
        case TK_INT:
            res->exprType = newSimpType( T_INT, r );
            RES_INT_VAL_LVAL( res ) = atoi( expr->text );
            break;
        case TK_DOUBLE:
            res->exprType = newSimpType( T_DOUBLE, r );
            RES_DOUBLE_VAL_LVAL( res ) = atof( expr->text );
            break;
        case TK_STRING:
            res = newStringRes( r, expr->text );
            break;
        case TK_VAR:
            res = evaluateVar3( expr->text, expr, rei, env, errmsg, r );
            break;
        case TK_TEXT:
            fd = ( FunctionDesc * )lookupFromEnv( ruleEngineConfig.extFuncDescIndex, expr->text );
            if ( fd != NULL && fd->exprType != NULL ) {
                int nArgs = 0;
                ExprType *type = fd->exprType;
                while ( getNodeType( type ) == T_CONS && strcmp( type->text, FUNC ) == 0 ) {
                    type = type->subtrees[1];
                    nArgs ++;
                }
                if ( nArgs == 0 ) {
                    Node *appNode = newPartialApplication( expr, newTupleRes( 0, NULL, r ), 0, r );
                    res = evaluateFunction3( appNode, applyAll, expr, env, rei, reiSaveFlag, errmsg, r );
                }
                else {
                    res = newFuncSymLink( expr->text, nArgs, r );
                }
            }
            else {
                res = newFuncSymLink( expr->text, 1, r );
            }
            break;


        case N_APPLICATION:
            /* try to evaluate as a function, */
            /*
                                    printf("start execing %s\n", oper1);
                                    printEnvToStdOut(env);

            */
            funcRes = evaluateExpression3( expr->subtrees[0], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg, r );
            if ( getNodeType( funcRes ) == N_ERROR ) {
                res = funcRes;
                break;
            }
            /* printTree(expr->subtrees[1], 0); */
            argRes = evaluateExpression3( expr->subtrees[1], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg, r );
            if ( getNodeType( argRes ) == N_ERROR ) {
                res = argRes;
                break;
            }
            res = evaluateFunctionApplication( funcRes, argRes, applyAll, expr, rei, reiSaveFlag, env, errmsg, r );
            /*
                                    printf("finish execing %s\n", oper1);
                                    printEnvToStdOut(env);
            */
            break;
        case N_TUPLE:
            tupleComps = ( Res ** ) region_alloc( r, sizeof( Res * ) * expr->degree );
            for ( i = 0; i < expr->degree; i++ ) {
                res = tupleComps[i] = evaluateExpression3( expr->subtrees[i], applyAll > 1 ? applyAll : 0, 0, rei, reiSaveFlag, env, errmsg, r );
                if ( getNodeType( res ) == N_ERROR ) {
                    break;
                }
            }
            if ( expr->degree == 0 || getNodeType( res ) != N_ERROR ) {
                if ( N_TUPLE_CONSTRUCT_TUPLE( expr ) || expr->degree != 1 ) {
                    res = newTupleRes( expr->degree, tupleComps, r );
                }
            }
            break;
        case N_ACTIONS_RECOVERY:
            res = evaluateActions( expr->subtrees[0], expr->subtrees[1], applyAll, rei, reiSaveFlag, env, errmsg, r );
            break;

        case N_ACTIONS:
            generateErrMsg( "error: evaluate actions using function evaluateExpression3, use function evaluateActions instead.", NODE_EXPR_POS( expr ), expr->base, errbuf );
            addRErrorMsg( errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf );
            res = newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
            break;
        default:
            generateErrMsg( "error: unsupported ast node type.", NODE_EXPR_POS( expr ), expr->base, errbuf );
            addRErrorMsg( errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf );
            res = newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
            break;
        }
    }
    else {
        res = expr;
        while ( getNodeType( res ) == N_TUPLE && res->degree == 1 ) {
            res = res->subtrees[0];
        }
    }
    /* coercions are applied at application locations only */
    return res;
}

ExprType* isIterable( ExprType *type, Hashtable* var_type_table, Region *r );
Res* processCoercion( Node *node, Res *res, ExprType *type, Hashtable *tvarEnv, rError_t *errmsg, Region *r ) {
    char buf[ERR_MSG_LEN > 1024 ? ERR_MSG_LEN : 1024];
    char *buf2;
    char buf3[ERR_MSG_LEN];
    ExprType *coercion = type;
    ExprType *futureCoercion = NULL;
    if ( getNodeType( coercion ) == T_FLEX ) {
        coercion = coercion->subtrees[0];
    }
    else if ( getNodeType( coercion ) == T_FIXD ) {
        coercion = coercion->subtrees[1];
    }
    if ( coercion->exprType != NULL ) {
        futureCoercion = coercion->exprType;
    }
    coercion = instantiate( coercion, tvarEnv, 0, r );
    if ( getNodeType( coercion ) == T_VAR ) {
        if ( T_VAR_NUM_DISJUNCTS( coercion ) == 0 ) {
            /* generateErrMsg("error: cannot instantiate coercion type for node.", NODE_EXPR_POS(node), node->base, buf);
            addRErrorMsg(errmsg, -1, buf);
            return newErrorRes(r, -1); */
            return res;
        }
        /* here T_VAR must be a set of bounds
         * we fix the set of bounds to the default bound */
        ExprType *defaultType = T_VAR_DISJUNCT( coercion, 0 );
        updateInHashTable( tvarEnv, getTVarName( T_VAR_ID( coercion ), buf ), defaultType );
        coercion = defaultType;
    }
    Res *nres = NULL;
    if ( typeEqSyntatic( coercion, res->exprType ) ) {
        nres = res;
    }
    else {
        if ( TYPE( res ) == T_UNSPECED ) {
            generateErrMsg( "error: dynamic coercion from an uninitialized value", NODE_EXPR_POS( node ), node->base, buf );
            addRErrorMsg( errmsg, RE_DYNAMIC_COERCION_ERROR, buf );
            return newErrorRes( r, RE_DYNAMIC_COERCION_ERROR );
        }
        switch ( getNodeType( coercion ) ) {
        case T_DYNAMIC:
            nres = res;
            break;
        case T_INT:
            switch ( TYPE( res ) ) {
            case T_DOUBLE:
                if ( ( int )RES_DOUBLE_VAL( res ) != RES_DOUBLE_VAL( res ) ) {
                    generateErrMsg( "error: dynamic type conversion DOUBLE -> INTEGER: the double is not an integer", NODE_EXPR_POS( node ), node->base, buf );
                    addRErrorMsg( errmsg, RE_DYNAMIC_COERCION_ERROR, buf );
                    return newErrorRes( r, RE_DYNAMIC_COERCION_ERROR );
                }
                else {
                    nres = newIntRes( r, RES_INT_VAL( res ) );
                }
                break;
            case T_BOOL:
                nres = newIntRes( r, RES_BOOL_VAL( res ) );
                break;
            case T_STRING:
                nres = newIntRes( r, atoi( res->text ) );
                break;
            default:
                break;
            }
            break;
        case T_DOUBLE:
            switch ( TYPE( res ) ) {
            case T_INT:
                nres = newDoubleRes( r, RES_INT_VAL( res ) );
                break;
            case T_BOOL:
                nres = newDoubleRes( r, RES_BOOL_VAL( res ) );
                break;
            case T_STRING:
                nres = newDoubleRes( r, atof( res->text ) );
                break;
            default:
                break;
            }
            break;
        case T_STRING:
            switch ( TYPE( res ) ) {
            case T_INT:
            case T_DOUBLE:
            case T_BOOL:
                buf2 = convertResToString( res );

                nres = newStringRes( r, buf2 );
                free( buf2 );
                break;
            default:
                break;
            }
            break;
        case T_PATH:
            switch ( TYPE( res ) ) {
            case T_STRING:
                nres = newPathRes( r, res->text );
                break;
            default:
                break;
            }
            break;
        case T_BOOL:
            switch ( TYPE( res ) ) {
            case T_INT:
                nres = newBoolRes( r, RES_INT_VAL( res ) );
                break;
            case T_DOUBLE:
                nres = newBoolRes( r, ( int ) RES_DOUBLE_VAL( res ) );
                break;
            case T_STRING:
                if ( strcmp( res->text, "true" ) == 0 ) {
                    nres = newBoolRes( r, 1 );
                }
                else if ( strcmp( res->text, "false" ) == 0 ) {
                    nres = newBoolRes( r, 0 );
                }
                else {
                    generateErrMsg( "error: dynamic type conversion  string -> bool: the string is not in {true, false}", NODE_EXPR_POS( node ), node->base, buf );
                    addRErrorMsg( errmsg, RE_DYNAMIC_COERCION_ERROR, buf );
                    return newErrorRes( r, RE_DYNAMIC_COERCION_ERROR );
                }
                break;
            default:
                break;
            }
            break;
        case T_CONS:
            /* we can ignore the not top level type constructor and leave type checking to when it is derefed */
            switch ( TYPE( res ) ) {
            case T_CONS:
                nres = res;
                break;
            default:
                if ( isIterable( res->exprType, newHashTable2( 10, r ), r ) != NULL ) {
                    nres = res;
                }
                break;
            }
            break;
        case T_DATETIME:
            switch ( TYPE( res ) ) {
            case T_INT:
                nres = newDatetimeRes( r, ( time_t ) RES_INT_VAL( res ) );
                break;
            case T_DOUBLE:
                nres = newDatetimeRes( r, ( time_t ) RES_DOUBLE_VAL( res ) );
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
    char typeBuf1[128], typeBuf2[128];
    if ( nres != NULL ) {
        if ( futureCoercion != NULL ) {
            if ( getNodeType( futureCoercion ) != T_VAR || futureCoercion->degree == 0 ) {
                snprintf( buf, ERR_MSG_LEN, "error: flexible coercion target type supported only for union types, but is applied to %s",
                          typeToString( futureCoercion, tvarEnv, typeBuf1, 128 ) );
                generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf3 );
                addRErrorMsg( errmsg, RE_TYPE_ERROR, buf3 );
                return newErrorRes( r, RE_TYPE_ERROR );
            }
            switch ( TYPE( nres ) ) {
            case T_INT:
                nres = newIntRes( r, RES_INT_VAL( nres ) );
                nres->exprType->exprType = futureCoercion;
                break;
            case T_DOUBLE:
                nres = newDoubleRes( r, RES_DOUBLE_VAL( nres ) );
                nres->exprType->exprType = futureCoercion;
                break;
            default:
                snprintf( buf, ERR_MSG_LEN, "error: flexible coercion source type supported only for integer or double, but is applied to %s",
                          typeToString( nres->exprType, tvarEnv, typeBuf1, 128 ) );
                generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf3 );
                addRErrorMsg( errmsg, RE_TYPE_ERROR, buf3 );
                return newErrorRes( r, RE_TYPE_ERROR );
            }
        }
        return nres;
    }
    snprintf( buf, ERR_MSG_LEN, "error: coerce from type %s to type %s",
              typeToString( res->exprType, tvarEnv, typeBuf1, 128 ),
              typeToString( coercion, tvarEnv, typeBuf2, 128 ) );
    generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf3 );
    addRErrorMsg( errmsg, RE_TYPE_ERROR, buf3 );
    return newErrorRes( r, RE_TYPE_ERROR );
}

Res* evaluateActions( Node *expr, Node *reco, int applyAll, ruleExecInfo_t *rei, int reiSaveFlag, Env *env, rError_t* errmsg, Region *r ) {
    /*
        printTree(expr, 0);
    */
    int i;
    int cutFlag = 0;
    Res* res = NULL;
#ifndef DEBUG
    char tmpStr[1024];
#endif
    switch ( getNodeType( expr ) ) {
    case N_ACTIONS:
        for ( i = 0; i < expr->degree; i++ ) {
            Node *nodei = expr->subtrees[i];
            if ( getNodeType( nodei ) == N_APPLICATION && getNodeType( nodei->subtrees[0] ) == TK_TEXT && strcmp( nodei->subtrees[0]->text, "cut" ) == 0 ) {
                cutFlag = 1;
                continue;
            }
            res = evaluateExpression3( nodei, applyAll, 0, rei, reiSaveFlag, env, errmsg, r );
            if ( getNodeType( res ) == N_ERROR ) {
#ifndef DEBUG
                char *errAction = getNodeType( nodei ) == N_APPLICATION ? N_APP_FUNC( nodei )->text : nodei->text;
                sprintf( tmpStr, "executeRuleAction Failed for %s", errAction );
                rodsLogError( LOG_ERROR, RES_ERR_CODE( res ), tmpStr );
                rodsLog( LOG_NOTICE, "executeRuleBody: Microservice or Action %s Failed with status %i", errAction, RES_ERR_CODE( res ) );
#endif
                /* run recovery chain */
                if ( RES_ERR_CODE( res ) != RETRY_WITHOUT_RECOVERY_ERR && reco != NULL ) {
                    int i2;
                    for ( i2 = reco->degree - 1 < i ? reco->degree - 1 : i; i2 >= 0; i2-- ) {
#ifndef DEBUG
                        if ( reTestFlag > 0 ) {
                            if ( reTestFlag == COMMAND_TEST_1 || COMMAND_TEST_MSI ) {
                                fprintf( stdout, "***RollingBack\n" );
                            }
                            else if ( reTestFlag == HTML_TEST_1 ) {
                                fprintf( stdout, "<FONT COLOR=#FF0000>***RollingBack</FONT><BR>\n" );
                            }
                            else  if ( reTestFlag == LOG_TEST_1 )
                                if ( rei != NULL && rei->rsComm != NULL && &( rei->rsComm->rError ) != NULL ) {
                                    rodsLog( LOG_NOTICE, "***RollingBack\n" );
                                }
                        }
#endif

                        Res *res2 = evaluateExpression3( reco->subtrees[i2], 0, 0, rei, reiSaveFlag, env, errmsg, r );
                        if ( getNodeType( res2 ) == N_ERROR ) {
#ifndef DEBUG
                            char *errAction = getNodeType( reco->subtrees[i2] ) == N_APPLICATION ? N_APP_FUNC( reco->subtrees[i2] )->text : reco->subtrees[i2]->text;
                            sprintf( tmpStr, "executeRuleRecovery Failed for %s", errAction );
                            rodsLogError( LOG_ERROR, RES_ERR_CODE( res2 ), tmpStr );
#endif
                        }
                    }
                }
                if ( cutFlag ) {
                    return newErrorRes( r, CUT_ACTION_PROCESSED_ERR );
                }
                else {
                    return res;
                }
            }
            else if ( TYPE( res ) == T_BREAK ) {
                return res;
            }
            else if ( TYPE( res ) == T_SUCCESS ) {
                return res;
            }

        }
        return res == NULL ? newIntRes( r, 0 ) : res;
    default:
        break;
    }
    char errbuf[ERR_MSG_LEN];
    generateErrMsg( "error: unsupported ast node type.", NODE_EXPR_POS( expr ), expr->base, errbuf );
    addRErrorMsg( errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf );
    return newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
}

Res *evaluateFunctionApplication( Node *func, Node *arg, int applyAll, Node *node, ruleExecInfo_t* rei, int reiSaveFlag, Env *env, rError_t *errmsg, Region *r ) {
    Res *res;
    char errbuf[ERR_MSG_LEN];
    switch ( getNodeType( func ) ) {
    case N_SYM_LINK:
    case N_PARTIAL_APPLICATION:
        res = newPartialApplication( func, arg, RES_FUNC_N_ARGS( func ) - 1, r );
        if ( RES_FUNC_N_ARGS( res ) == 0 ) {
            res = evaluateFunction3( res, applyAll, node, env, rei, reiSaveFlag, errmsg, r );
        }
        return res;
    default:
        generateErrMsg( "unsupported function node type.", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, RE_UNSUPPORTED_AST_NODE_TYPE, errbuf );
        return newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
    }
}

/**
 * evaluate function
 * provide env and region isolation for rules and external microservices
 * precond n <= MAX_PARAMS_LEN
 */
Res* evaluateFunction3( Node *appRes, int applyAll, Node *node, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    unsigned int i;
    unsigned int n;
    Node* args[MAX_FUNC_PARAMS];
    Node* argsProcessed[MAX_FUNC_PARAMS];
    i = 0;
    Node *appFuncRes = appRes;
    while ( getNodeType( appFuncRes ) == N_PARTIAL_APPLICATION ) {
        i++;
        appFuncRes = appFuncRes->subtrees[0];
    }
    /* app can only be N_FUNC_SYM_LINK */
    char* fn = appFuncRes->text;
    if ( strcmp( fn, "nop" ) == 0 ) {
        return newIntRes( r, 0 );
    }
    /*    printEnvIndent(env);
        printf("calling function %s\n", fn);
        char buf0[ERR_MSG_LEN];
        generateErrMsg("", NODE_EXPR_POS(node), node->base, buf0);
        printf("%s", buf0); */
    Res *appArgRes = appRes->subtrees[1];

    n = appArgRes->degree;
    Res** appArgs = appArgRes->subtrees;
    Node** nodeArgs = node->subtrees[1]->subtrees;
    ExprType *coercionType = NULL;
#ifdef DEBUG
    char buf[ERR_MSG_LEN > 1024 ? ERR_MSG_LEN : 1024];
    sprintf( buf, "Action: %s\n", fn );
    writeToTmp( "eval.log", buf );
#endif
    /* char buf2[ERR_MSG_LEN]; */

    Res* res;
    Region *newRegion = make_region( 0, NULL );
    Env *global = globalEnv( env );
    Env *nEnv = newEnv( newHashTable2( 10, newRegion ), global, env, newRegion );

    List *localTypingConstraints = NULL;
    FunctionDesc *fd = NULL;
    /* look up function descriptor */
    fd = ( FunctionDesc * )lookupFromEnv( ruleEngineConfig.extFuncDescIndex, fn );

    localTypingConstraints = newList( r );
    int ioParam[MAX_FUNC_PARAMS];
    /* evaluation parameters and try to resolve remaining tvars with unification */
    for ( i = 0; i < n; i++ ) {
        switch ( getIOType( nodeArgs[i] ) ) {
        case IO_TYPE_INPUT | IO_TYPE_OUTPUT: /* input/output */
            ioParam[i] = IO_TYPE_INPUT | IO_TYPE_OUTPUT;
            if ( !isVariableNode( appArgs[i] ) ) {
                res = newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
                generateAndAddErrMsg( "unsupported output parameter type", appArgs[i], RE_UNSUPPORTED_AST_NODE_TYPE, errmsg );
                RETURN;
            }
            args[i] = evaluateExpression3( appArgs[i], applyAll > 1 ? applyAll : 0, 1, rei, reiSaveFlag, env, errmsg, newRegion );
            if ( getNodeType( args[i] ) == N_ERROR ) {
                res = ( Res * )args[i];
                RETURN;
            }
            break;
        case IO_TYPE_INPUT: /* input */
            ioParam[i] = IO_TYPE_INPUT;
            args[i] = appArgs[i];
            break;
        case IO_TYPE_DYNAMIC: /* dynamic */
            if ( isVariableNode( appArgs[i] ) ) {
                args[i] = attemptToEvaluateVar3( appArgs[i]->text, appArgs[i], rei, env, errmsg, newRegion );
                if ( getNodeType( args[i] ) == N_ERROR ) {
                    res = args[i];
                    RETURN;
                }
                else if ( TYPE( args[i] ) == T_UNSPECED ) {
                    ioParam[i] = IO_TYPE_OUTPUT;
                }
                else {
                    ioParam[i] = IO_TYPE_INPUT | IO_TYPE_OUTPUT;
                    if ( getNodeType( args[i] ) == N_ERROR ) {
                        res = ( Res * )args[i];
                        RETURN;
                    }
                }
            }
            else {
                ioParam[i] = IO_TYPE_INPUT;
                args[i] = evaluateExpression3( appArgs[i], applyAll > 1 ? applyAll : 0, 1, rei, reiSaveFlag, env, errmsg, newRegion );
                if ( getNodeType( args[i] ) == N_ERROR ) {
                    res = ( Res * )args[i];
                    RETURN;
                }
            }
            break;
        case IO_TYPE_OUTPUT: /* output */
            ioParam[i] = IO_TYPE_OUTPUT;
            args[i] = newUnspecifiedRes( r );
            break;
        case IO_TYPE_EXPRESSION: /* expression */
            ioParam[i] = IO_TYPE_EXPRESSION;
            args[i] = appArgs[i];
            break;
        case IO_TYPE_ACTIONS: /* actions */
            ioParam[i] = IO_TYPE_ACTIONS;
            args[i] = appArgs[i];
            break;
        }
    }
    /* try to type all input parameters */
    coercionType = node->subtrees[1]->coercionType;
    if ( coercionType != NULL ) {
        /*for(i=0;i<n;i++) {
             if((ioParam[i] == 'i' || ioParam[i] == 'p') && nodeArgs[i]->exprType != NULL) {
                if(unifyWith(args[i]->exprType, nodeArgs[i]->exprType, env->current, r) == NULL) {
                    snprintf(buf, ERR_MSG_LEN, "error: dynamically typed parameter does not have expected type.");
                                        generateErrMsg(buf, nodeArgs[i]->expr, nodeArgs[i]->base, buf2);
                                        addRErrorMsg(errmsg, TYPE_ERROR, buf2);
                                        res = newErrorRes(r, TYPE_ERROR);
                                        RETURN;
                }
            }
        }*/


        ExprType *argType = newTupleRes( n, args, r )->exprType;
        if ( typeFuncParam( node->subtrees[1], argType, coercionType, env->current, localTypingConstraints, errmsg, newRegion ) != 0 ) {
            res = newErrorRes( r, RE_TYPE_ERROR );
            RETURN;
        }
        /* solve local typing constraints */
        /* typingConstraintsToString(localTypingConstraints, buf, 1024);
        printf("%s\n", buf); */
        Node *errnode;
        if ( !solveConstraints( localTypingConstraints, env->current, errmsg, &errnode, r ) ) {
            res = newErrorRes( r, RE_DYNAMIC_TYPE_ERROR );
            RETURN;
        }
        /*printVarTypeEnvToStdOut(localTVarEnv); */
        /* do the input value conversion */
        ExprType **coercionTypes = coercionType->subtrees;
        for ( i = 0; i < n; i++ ) {
            if ( ( ( ioParam[i] & IO_TYPE_INPUT ) == IO_TYPE_INPUT ) && ( nodeArgs[i]->option & OPTION_COERCE ) != 0 ) {
                argsProcessed[i] = processCoercion( nodeArgs[i], args[i], coercionTypes[i], env->current, errmsg, newRegion );
                if ( getNodeType( argsProcessed[i] ) == N_ERROR ) {
                    res = ( Res * )argsProcessed[i];
                    RETURN;
                }
            }
            else {
                argsProcessed[i] = args[i];
            }
        }
    }
    else {
        memcpy( argsProcessed, args, sizeof( Res * ) * n );
    }


    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = fn;
        param.ruleIndex = -1;
        reDebug( EXEC_ACTION_BEGIN, -4, &param, node, env, rei );
    }

    if ( fd != NULL ) {
        switch ( getNodeType( fd ) ) {
        case N_FD_DECONSTRUCTOR:
            res = deconstruct( argsProcessed, FD_PROJ( fd ) );
            break;
        case N_FD_CONSTRUCTOR:
            res = construct( fn, argsProcessed, n, instantiate( node->exprType, env->current, 1, r ), r );
            break;
        case N_FD_FUNCTION:
            res = ( Res * ) FD_SMSI_FUNC_PTR( fd )( argsProcessed, n, node, rei, reiSaveFlag,  env, errmsg, newRegion );
            break;
        case N_FD_EXTERNAL:
            res = execAction3( fn, argsProcessed, n, applyAll, node, nEnv, rei, reiSaveFlag, errmsg, newRegion );
            break;
        case N_FD_RULE_INDEX_LIST:
            res = execAction3( fn, argsProcessed, n, applyAll, node, nEnv, rei, reiSaveFlag, errmsg, newRegion );
            break;
        default:
            res = newErrorRes( r, RE_UNSUPPORTED_AST_NODE_TYPE );
            generateAndAddErrMsg( "unsupported function descriptor type", node, RE_UNSUPPORTED_AST_NODE_TYPE, errmsg );
            RETURN;
        }
    }
    else {
        res = execAction3( fn, argsProcessed, n, applyAll, node, nEnv, rei, reiSaveFlag, errmsg, newRegion );
    }

    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = fn;
        param.ruleIndex = -1;

        reDebug( EXEC_ACTION_END, -4, &param, node, env, rei );
    }

    if ( getNodeType( res ) == N_ERROR ) {
        RETURN;
    }

    for ( i = 0; i < n; i++ ) {
        Res *resp = NULL;

        if ( ( ioParam[i] & IO_TYPE_OUTPUT ) == IO_TYPE_OUTPUT ) {
            if ( ( appArgs[i]->option & OPTION_COERCE ) != 0 ) {
                argsProcessed[i] = processCoercion( nodeArgs[i], argsProcessed[i], appArgs[i]->exprType, env->current, errmsg, newRegion );
            }
            if ( getNodeType( argsProcessed[i] ) == N_ERROR ) {
                res = ( Res * )argsProcessed[i];
                RETURN ;
            }
            if ( ( ioParam[i] & IO_TYPE_INPUT ) == 0 || !definitelyEq( args[i], argsProcessed[i] ) ) {
                resp = setVariableValue( appArgs[i]->text, argsProcessed[i], nodeArgs[i], rei, env, errmsg, r );
            }
            /*char *buf = convertResToString(args[i]);
            printEnvIndent(env);
            printf("setting variable %s to %s\n", appArgs[i]->text, buf);
            free(buf);*/
        }
        if ( resp != NULL && getNodeType( resp ) == N_ERROR ) {
            res = resp;
            RETURN;
        }
    }
    /*printEnvIndent(env);
    printf("exiting function %s\n", fn);
    */
    /*return res;*/
ret:
    /*deleteEnv(nEnv, 2);*/
    cpEnv2( env, newRegion, r );
    res = cpRes2( res, newRegion, r );
    region_free( newRegion );
    return res;

}

Res* attemptToEvaluateVar3( char* vn, Node *node, ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r ) {
    if ( vn[0] == '*' ) { /* local variable */
        /* try to get local var from env */
        Res* res0 = ( Res * )lookupFromEnv( env, vn );
        if ( res0 == NULL ) {
            return newUnspecifiedRes( r );
        }
        else {
            return res0;
        }
    }
    else if ( vn[0] == '$' ) { /* session variable */
        Res *res = getSessionVar( "", node, vn, rei, errmsg, r );
        if ( res == NULL ) {
            return newUnspecifiedRes( r );
        }
        else {
            return res;
        }
    }
    else {
        return NULL;
    }
}

Res* evaluateVar3( char* vn, Node *node, ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r ) {
    char buf[ERR_MSG_LEN > 1024 ? ERR_MSG_LEN : 1024];
    char buf2[ERR_MSG_LEN];
    Res *res = attemptToEvaluateVar3( vn, node, rei, env, errmsg, r );
    if ( getNodeType( res ) == N_ERROR ) {
        return res;
    }
    if ( res == NULL || TYPE( res ) == T_UNSPECED ) {
        if ( vn[0] == '*' ) { /* local variable */
            snprintf( buf, ERR_MSG_LEN, "error: unable to read local variable %s.", vn );
            generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf2 );
            addRErrorMsg( errmsg, RE_UNABLE_TO_READ_LOCAL_VAR, buf2 );
            /*printEnvToStdOut(env); */
            return newErrorRes( r, RE_UNABLE_TO_READ_LOCAL_VAR );
        }
        else if ( vn[0] == '$' ) { /* session variable */
            snprintf( buf, ERR_MSG_LEN, "error: unable to read session variable %s.", vn );
            generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf2 );
            addRErrorMsg( errmsg, RE_UNABLE_TO_READ_SESSION_VAR, buf2 );
            return newErrorRes( r, RE_UNABLE_TO_READ_SESSION_VAR );
        }
        else {
            snprintf( buf, ERR_MSG_LEN, "error: unable to read variable %s.", vn );
            generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf2 );
            addRErrorMsg( errmsg, RE_UNABLE_TO_READ_VAR, buf2 );
            return newErrorRes( r, RE_UNABLE_TO_READ_VAR );
        }
    }
    return res;
}

/**
 * return NULL error
 *        otherwise success
 */
Res* getSessionVar( char *action,  Node *node, char *varName,  ruleExecInfo_t *rei, rError_t *errmsg, Region *r ) {
    /* Maps varName to the standard name and make varMap point to it. */
    /* It seems that for each pair of varName and standard name there is a list of actions that are supported. */
    /* vinx stores the index of the current pair so that we can start for the next pair if the current pair fails. */
    char *varMap = NULL;
    for ( int vinx = getVarMap( action, varName, &varMap, 0 ); vinx >= 0;
            vinx = getVarMap( action, varName, &varMap, vinx + 1 ) ) {
        /* Get the value of session variable referenced by varMap. */
        Res *varValue = NULL;
        int i = getVarValue( varMap, rei, &varValue, r ); /* reVariableMap.c */
        if ( i >= 0 ) {

            FunctionDesc *fd = ( FunctionDesc * ) lookupFromEnv( ruleEngineConfig.extFuncDescIndex, varMap );
            if ( fd != NULL ) {
                ExprType *type = fd->exprType->subtrees[0]; /* get var type from varMap */
                Hashtable *tvarEnv = newHashTable2( 10, r );
                varValue = processCoercion( node, varValue, type, tvarEnv, errmsg, r );
            }
            free( varMap );
            return varValue;
        }
        else if ( i != NULL_VALUE_ERR ) {  /* On error, return 0. */
            free( varMap );
            return NULL;
        }
        free( varMap );
        varMap = NULL;
        /* Try next varMap */
    }
    free( varMap );
    return NULL;
}

/*
 * execute an external microserive or a rule
 */
Res* execAction3( char *actionName, Res** args, unsigned int nargs, int applyAllRule, Node *node, Env *env, ruleExecInfo_t* rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    char buf[ERR_MSG_LEN > 1024 ? ERR_MSG_LEN : 1024];
    char buf2[ERR_MSG_LEN];
    char action[MAX_NAME_LEN];
    snprintf( action, sizeof( action ), "%s", actionName );
    mapExternalFuncToInternalProc2( action );

    /* no action (microservice) found, try to lookup a rule */
    Res *actionRet = execRule( actionName, args, nargs, applyAllRule, env, rei, reiSaveFlag, errmsg, r );
    if ( getNodeType( actionRet ) == N_ERROR && (
                RES_ERR_CODE( actionRet ) == NO_RULE_FOUND_ERR ) ) {

        // =-=-=-=-=-=-=-
        // didnt find a rule, try a msvc
        irods::ms_table_entry ms_entry;
        int actionInx = actionTableLookUp( ms_entry, action );
        if ( actionInx >= 0 ) { /* rule */

            return execMicroService3( action, args, nargs, node, env, rei, errmsg, r );
        }
        else {
            snprintf( buf, ERR_MSG_LEN, "error: cannot find rule for action \"%s\" available: %d.", action, availableRules() );
            generateErrMsg( buf, NODE_EXPR_POS( node ), node->base, buf2 );
            addRErrorMsg( errmsg, NO_RULE_OR_MSI_FUNCTION_FOUND_ERR, buf2 );
            return newErrorRes( r, NO_RULE_OR_MSI_FUNCTION_FOUND_ERR );
        }
    }
    else {
        return actionRet;
    }
}



/**
 * execute micro service msiName
 */
Res* execMicroService3( char *msName, Res **args, unsigned int nargs, Node *node, Env *env, ruleExecInfo_t *rei, rError_t *errmsg, Region *r ) {
    msParamArray_t *origMsParamArray = rei->msParamArray;
    funcPtr myFunc = NULL;
    int actionInx;
    unsigned int numOfStrArgs;
    unsigned int i;
    int ii = 0;
    msParam_t *myArgv[MAX_PARAMS_LEN];
    Res *res;

    /* look up the micro service */
    irods::ms_table_entry ms_entry;
    actionInx = actionTableLookUp( ms_entry, msName );

    char errbuf[ERR_MSG_LEN];
    if ( actionInx < 0 ) {
        int ret = NO_MICROSERVICE_FOUND_ERR;
        generateErrMsg( "execMicroService3: no micro service found", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, ret, errbuf );
        return newErrorRes( r, ret );

    }

    myFunc       = ms_entry.call_action_;
    numOfStrArgs = ms_entry.num_args_;
    if ( nargs != numOfStrArgs ) {
        int ret = ACTION_ARG_COUNT_MISMATCH;
        generateErrMsg( "execMicroService3: wrong number of arguments", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, ret, errbuf );
        return newErrorRes( r, ret );
    }

    /* convert arguments from Res to msParam_t */
    /* char buf[1024]; */
    int fillInParamLabel = node->degree == 2 && node->subtrees[1]->degree == ( int ) numOfStrArgs;
    for ( i = 0; i < numOfStrArgs; i++ ) {
        myArgv[i] = ( msParam_t * )malloc( sizeof( msParam_t ) );
        Res *res = args[i];
        if ( res != NULL ) {
            int ret =
                convertResToMsParam( myArgv[i], res, errmsg );
            if ( ret != 0 ) {
                generateErrMsg( "execMicroService3: error converting arguments to MsParam", NODE_EXPR_POS( node->subtrees[1]->subtrees[i] ), node->subtrees[1]->subtrees[i]->base, errbuf );
                addRErrorMsg( errmsg, ret, errbuf );
                int j;
                for ( j = i - 1; j >= 0; j-- ) {
                    int freeStruct = TYPE( args[j] ) != T_IRODS  ? 1 : 0;
                    clearMsParam( myArgv[j], freeStruct );
                    free( myArgv[j] );
                }
                return newErrorRes( r, ret );
            }
            myArgv[i]->label = fillInParamLabel && isVariableNode( node->subtrees[1]->subtrees[i] ) ? strdup( node->subtrees[1]->subtrees[i]->text ) : NULL;
        }
        else {
            myArgv[i]->inOutStruct = NULL;
            myArgv[i]->inpOutBuf = NULL;
            myArgv[i]->type = strdup( STR_MS_T );
        }
        /* sprintf(buf,"**%d",i); */
        /* myArgv[i]->label = strdup(buf); */
    }
    /* convert env to msparam array */
    rei->msParamArray = newMsParamArray();
    int ret = convertEnvToMsParamArray( rei->msParamArray, env, errmsg, r );
    if ( ret != 0 ) {
        generateErrMsg( "execMicroService3: error converting env to MsParamArray", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, ret, errbuf );
        res = newErrorRes( r, ret );
        RETURN;
    }

    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = msName;
        param.ruleIndex = -1;
        reDebug( EXEC_MICRO_SERVICE_BEGIN, -4, &param, node, env, rei );
    }

    if ( numOfStrArgs == 0 ) {
        ii = ( *( int ( * )( ruleExecInfo_t * ) )myFunc )( rei ) ;
    }
    else if ( numOfStrArgs == 1 ) {
        ii = ( *( int ( * )( msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], rei );
    }
    else if ( numOfStrArgs == 2 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], rei );
    }
    else if ( numOfStrArgs == 3 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], rei );
    }
    else if ( numOfStrArgs == 4 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], rei );
    }
    else if ( numOfStrArgs == 5 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], rei );
    }
    else if ( numOfStrArgs == 6 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], rei );
    }
    else if ( numOfStrArgs == 7 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], rei );
    }
    else if ( numOfStrArgs == 8 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7], rei );
    }
    else if ( numOfStrArgs == 9 ) {
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7], myArgv[8], rei );
    }
    else if ( numOfStrArgs == 10 )
        ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7],
                myArgv[8], myArgv [9], rei );

    /* move errmsgs from rei to errmsg */
    if ( rei->rsComm != NULL ) {
        replErrorStack( &rei->rsComm->rError, errmsg );
        freeRErrorContent( &rei->rsComm->rError );
    }

    if ( ii < 0 ) {
        res = newErrorRes( r, ii );
        RETURN;
    }

    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = msName;
        param.ruleIndex = -1;
        reDebug( EXEC_MICRO_SERVICE_END, -4, &param, node, env, rei );
    }

    /* converts back env */
    ret = updateMsParamArrayToEnvAndFreeNonIRODSType( rei->msParamArray, env, r );
    if ( ret != 0 ) {
        generateErrMsg( "execMicroService3: error env from MsParamArray", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, ret, errbuf );
        res = newErrorRes( r, ret );
        RETURN;
    }
    /* params */
    for ( i = 0; i < numOfStrArgs; i++ ) {
        if ( myArgv[i] != NULL ) {
            res = convertMsParamToRes( myArgv[i], r );
            if ( res != NULL && getNodeType( res ) == N_ERROR ) {
                generateErrMsg( "execMicroService3: error converting arguments from MsParam", NODE_EXPR_POS( node ), node->base, errbuf );
                addRErrorMsg( errmsg, ret, errbuf );
                RETURN;
            }
            args[i] = res;
        }
        else {
            args[i] = NULL;
        }
    }

    /*
            char vn[100];
        for(i=0;i<numOfStrArgs;i++) {
                sprintf(vn,"**%d",i);
                largs[i] = lookupFromHashTable(env, vn);
        }
    */
    res = newIntRes( r, ii );
ret:
    if ( rei->msParamArray != NULL ) {
        deleteMsParamArray( rei->msParamArray );
    }
    rei->msParamArray = origMsParamArray;
    for ( i = 0; i < numOfStrArgs; i++ ) {
        int freeStruct = TYPE( args[i] ) != T_IRODS  ? 1 : 0;
        clearMsParam( myArgv[i], freeStruct );
        free( myArgv[i] );
    }
    if ( getNodeType( res ) == N_ERROR ) {
        generateErrMsg( "execMicroService3: error when executing microservice", NODE_EXPR_POS( node ), node->base, errbuf );
        addRErrorMsg( errmsg, RES_ERR_CODE( res ), errbuf );
    }
    return res;
}

Res* execRuleFromCondIndex( char *ruleName, Res **args, int argc, CondIndexVal *civ, int applyAll, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    /*printTree(civ->condExp, 0); */
    Res *status;
    Env *envNew = newEnv( newHashTable2( 10, r ), globalEnv( env ), env, r );
    RuleDesc *rd;
    Node* rule = NULL;
    RuleIndexListNode *indexNode = NULL;
    Res* res = NULL;


    if ( civ->params->degree != argc ) {
        char buf[ERR_MSG_LEN];
        snprintf( buf, ERR_MSG_LEN, "error: cannot apply rule %s from rule conditional index because of wrong number of arguments, declared %d, supplied %d.", ruleName, civ->params->degree, argc );
        addRErrorMsg( errmsg, ACTION_ARG_COUNT_MISMATCH, buf );
        return newErrorRes( r, ACTION_ARG_COUNT_MISMATCH );
    }

    int i = initializeEnv( civ->params, args, argc, envNew->current );
    if ( i != 0 ) {
        status = newErrorRes( r, i );
        RETURN;
    }

    res = evaluateExpression3( civ->condExp, 0, 0, rei, reiSaveFlag, envNew,  errmsg, r );
    if ( getNodeType( res ) == N_ERROR ) {
        status = res;
        RETURN;
    }
    if ( TYPE( res ) != T_STRING ) {
        /* todo try coercion */
        addRErrorMsg( errmsg, RE_DYNAMIC_TYPE_ERROR, "error: the lhs of indexed rule condition does not evaluate to a string" );
        status = newErrorRes( r, RE_DYNAMIC_TYPE_ERROR );
        RETURN;
    }

    indexNode = ( RuleIndexListNode * )lookupFromHashTable( civ->valIndex, res->text );
    if ( indexNode == NULL ) {
#ifndef DEBUG
        rodsLog( LOG_NOTICE, "cannot find rule in condIndex: %s", ruleName );
#endif
        status = newErrorRes( r, NO_MORE_RULES_ERR );
        RETURN;
    }

    rd = getRuleDesc( indexNode->ruleIndex ); /* increase the index to move it to core rules index below MAX_NUM_APP_RULES are app rules */

    if ( rd->ruleType != RK_REL && rd->ruleType != RK_FUNC ) {
#ifndef DEBUG
        rodsLog( LOG_NOTICE, "wrong node type in condIndex: %s", ruleName );
#endif
        status = newErrorRes( r, NO_MORE_RULES_ERR );
        RETURN;
    }

    rule = rd->node;

    status = execRuleNodeRes( rule, args, argc,  applyAll > 1 ? applyAll : 0, env, rei, reiSaveFlag, errmsg, r );

    if ( getNodeType( status ) == N_ERROR ) {
        rodsLog( LOG_NOTICE, "execRuleFromCondIndex: applyRule Failed: %s with status %i", ruleName, RES_ERR_CODE( status ) );
    }

ret:
    /* deleteEnv(envNew, 2); */
    return status;


}
#define SYSTEM_SPACE_RULE 0x100
/*
 * look up rule node by rulename from index
 * apply rule condition index if possible
 * call execRuleNodeRes until success or no more rules
 */
Res *execRule( char *ruleNameInp, Res** args, unsigned int argc, int applyAllRuleInp, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    int ruleInx = 0, statusI;
    Res *statusRes = NULL;
    int  inited = 0;
    ruleExecInfo_t  *saveRei = NULL;
    int reTryWithoutRecovery = 0;
    char ruleName[MAX_NAME_LEN];
    int applyAllRule = applyAllRuleInp;

#ifdef DEBUG
    char buf[1024];
    snprintf( buf, 1024, "execRule: %s\n", ruleNameInp );
    writeToTmp( "entry.log", buf );
#endif

    ruleInx = 0; /* new rule */
    if ( strlen( ruleNameInp ) < sizeof( ruleName ) ) {
        snprintf( ruleName, sizeof( ruleName ), "%s", ruleNameInp );
    }
    else {
        rodsLog( LOG_ERROR, "ruleName: [%s] must be fewer than %ju characters in length.",
                 ruleNameInp, sizeof( ruleName ) );
        ruleName[0] = '\0';
    }

    mapExternalFuncToInternalProc2( ruleName );

    int systemSpaceRuleFlag = ( reiSaveFlag & SYSTEM_SPACE_RULE ) != 0 || lookupFromHashTable( ruleEngineConfig.coreFuncDescIndex->current, ruleName ) != NULL ? SYSTEM_SPACE_RULE : 0;
    int _reiSaveFlag = reiSaveFlag & SAVE_REI;

    RuleIndexListNode *ruleIndexListNode;
    int success = 0;
    int first = 1;
    while ( 1 ) {
        if ( systemSpaceRuleFlag != 0 ) {
            statusI = findNextRuleFromIndex( ruleEngineConfig.coreFuncDescIndex, ruleName, ruleInx, &ruleIndexListNode );
        }
        else {
            statusI = findNextRule2( ruleName, ruleInx, &ruleIndexListNode );
        }

        if ( statusI != 0 ) {
            if ( applyAllRule == 0 ) {
#ifndef DEBUG
//                rodsLog (LOG_NOTICE,"execRule: no more rules: %s with status %i",ruleName, statusI);
#endif
                statusRes = statusRes == NULL ? newErrorRes( r, NO_RULE_FOUND_ERR ) : statusRes;
            }
            else {   // apply all rules succeeds even when 0 rule is applied
                success = 1;
            }
            break;
        }
        if ( _reiSaveFlag == SAVE_REI ) {
            int statusCopy = 0;
            if ( inited == 0 ) {
                saveRei = ( ruleExecInfo_t * ) mallocAndZero( sizeof( ruleExecInfo_t ) );
                statusCopy = copyRuleExecInfo( rei, saveRei );
                inited = 1;
            }
            else if ( reTryWithoutRecovery == 0 ) {
                statusCopy = copyRuleExecInfo( saveRei, rei );
            }
            if ( statusCopy != 0 ) {
                statusRes = newErrorRes( r, statusCopy );
                break;
            }
        }

        if ( !first ) {
            addRErrorMsg( errmsg, statusI, ERR_MSG_SEP );
        }
        else {
            first = 0;
        }

        if ( ruleIndexListNode->secondaryIndex ) {
            statusRes = execRuleFromCondIndex( ruleName, args, argc, ruleIndexListNode->condIndex, applyAllRule, env, rei, reiSaveFlag | systemSpaceRuleFlag, errmsg, r );
        }
        else {

            RuleDesc *rd = getRuleDesc( ruleIndexListNode->ruleIndex );
            if ( rd->ruleType == RK_REL || rd->ruleType == RK_FUNC ) {

                Node* rule = rd->node;
                unsigned int inParamsCount = RULE_NODE_NUM_PARAMS( rule );
                if ( inParamsCount != argc ) {
                    ruleInx++;
                    continue;
                }

                if ( GlobalREAuditFlag > 0 ) {
                    RuleEngineEventParam param;
                    param.actionName = ruleName;
                    param.ruleIndex = ruleInx;
                    reDebug( GOT_RULE, 0, &param, rule, env, rei );

                }

#ifndef DEBUG
                if ( reTestFlag > 0 ) {
                    if ( reTestFlag == COMMAND_TEST_1 ) {
                        fprintf( stdout, "+Testing Rule Number:%i for Action:%s\n", ruleInx, ruleName );
                    }
                    else if ( reTestFlag == HTML_TEST_1 ) {
                        fprintf( stdout, "+Testing Rule Number:<FONT COLOR=#FF0000>%i</FONT> for Action:<FONT COLOR=#0000FF>%s</FONT><BR>\n", ruleInx, ruleName );
                    }
                    else if ( rei != 0 && rei->rsComm != 0 && &( rei->rsComm->rError ) != 0 ) {
                        rodsLog( LOG_NOTICE, "+Testing Rule Number:%i for Action:%s\n", ruleInx, ruleName );
                    }
                }
#endif
                /* printTree(rule, 0); */

                statusRes = execRuleNodeRes( rule, args, argc,  applyAllRule > 1 ? applyAllRule : 0, env, rei, reiSaveFlag | systemSpaceRuleFlag, errmsg, r );

            }
        }
        if ( statusRes != NULL && getNodeType( statusRes ) != N_ERROR ) {
            success = 1;
            if ( applyAllRule == 0 ) { /* apply first rule */
                break;
            }
            else {   /* apply all rules */
                if ( _reiSaveFlag == SAVE_REI ) {
                    freeRuleExecInfoStruct( saveRei, 0 );
                    inited = 0;
                }
            }
        }
        else if ( statusRes != NULL && RES_ERR_CODE( statusRes ) == RETRY_WITHOUT_RECOVERY_ERR ) {
            reTryWithoutRecovery = 1;
        }
        else if ( statusRes != NULL && RES_ERR_CODE( statusRes ) == CUT_ACTION_PROCESSED_ERR ) {
            break;
        }

        ruleInx++;
    }

    if ( inited == 1 ) {
        freeRuleExecInfoStruct( saveRei, 0 );
    }

    if ( success ) {
        if ( applyAllRule ) {
            /* if we apply all rules, then it succeeds even if some of the rules fail */
            return newIntRes( r, 0 );
        }
        else if ( statusRes != NULL && TYPE( statusRes ) == T_SUCCESS ) {
            return newIntRes( r, 0 );
        }
        else {
            return statusRes;
        }
    }
    else {
#ifndef DEBUG
//            rodsLog (LOG_NOTICE,"execRule: applyRule Failed: %s with status %i",ruleName, RES_ERR_CODE(statusRes));
#endif
        return statusRes;
    }
}
void copyFromEnv( Res **args, char **inParams, int inParamsCount, Hashtable *env, Region *r ) {
    for ( int i = 0; i < inParamsCount; i++ ) {
        if ( Res * res = ( Res * )lookupFromHashTable( env, inParams[i] ) ) {
            args[i] = cpRes( res, r );
        }
    }
}
/*
 * execute a rule given by an AST node
 * create a new environment and intialize it with parameters
 */
Res* execRuleNodeRes( Node *rule, Res** args, unsigned int argc, int applyAll, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    int restoreGlobalREAuditFlag = 0;
    int globalREAuditFlag = 0;
    if ( GlobalREAuditFlag > 0 ) {
        /* get meta data */
        Node *metadata = rule->subtrees[4];
        int i;
        for ( i = 0; i < metadata->degree; i++ ) {
            Node *attribute = metadata->subtrees[i]->subtrees[0];
            Node *value = metadata->subtrees[i]->subtrees[1];
            if ( strcmp( attribute->text, "logging" ) == 0 && strcmp( value->text, "false" ) == 0 ) {
                restoreGlobalREAuditFlag = 1;
                globalREAuditFlag = GlobalREAuditFlag;
                GlobalREAuditFlag = 0;
                break;
            }
        }

        RuleEngineEventParam param;
        param.actionName = RULE_NAME( rule );
        param.ruleIndex = -1;
        reDebug( EXEC_RULE_BEGIN, -4, &param, rule, env, rei ); /* pass in NULL for inMsParamArray for now */
    }
    Node* ruleCondition = rule->subtrees[1];
    Node* ruleAction = rule->subtrees[2];
    Node* ruleRecovery = rule->subtrees[3];
    Node* ruleHead = rule->subtrees[0];
    Node** paramsNodes = ruleHead->subtrees[0]->subtrees;
    char* paramsNames[MAX_NUM_OF_ARGS_IN_ACTION];
    unsigned int inParamsCount = RULE_NODE_NUM_PARAMS( rule );
    Res *statusRes;

    if ( inParamsCount != argc ) {
        generateAndAddErrMsg( "error: action argument count mismatch", rule, ACTION_ARG_COUNT_MISMATCH, errmsg );
        return newErrorRes( r, ACTION_ARG_COUNT_MISMATCH );

    }
    unsigned int k;
    for ( k = 0; k < inParamsCount ; k++ ) {
        paramsNames[k] =  paramsNodes[k]->text;
    }

    Env *global = globalEnv( env );
    Region *rNew = make_region( 0, NULL );
    Env *envNew = newEnv( newHashTable2( 10, rNew ), global, env, rNew );

    int statusInitEnv;
    /* printEnvToStdOut(envNew->current); */
    statusInitEnv = initializeEnv( ruleHead->subtrees[0], args, argc, envNew->current );
    if ( statusInitEnv != 0 ) {
        return newErrorRes( r, statusInitEnv );
    }
    /* printEnvToStdOut(envNew->current); */
    Res *res = evaluateExpression3( ruleCondition, 0, 0, rei, reiSaveFlag,  envNew, errmsg, rNew );
    /* todo consolidate every error into T_ERROR except OOM */
    if ( getNodeType( res ) != N_ERROR && TYPE( res ) == T_BOOL && RES_BOOL_VAL( res ) != 0 ) {
        if ( getNodeType( ruleAction ) == N_ACTIONS ) {
            statusRes = evaluateActions( ruleAction, ruleRecovery, applyAll, rei, reiSaveFlag,  envNew, errmsg, rNew );
        }
        else {
            statusRes = evaluateExpression3( ruleAction, applyAll, 0, rei, reiSaveFlag, envNew, errmsg, rNew );
        }
        /* copy parameters */
        copyFromEnv( args, paramsNames, inParamsCount, envNew->current, r );
        /* copy return values */
        statusRes = cpRes( statusRes, r );

        if ( getNodeType( statusRes ) == N_ERROR ) {
#ifndef DEBUG
            rodsLog( LOG_NOTICE, "execRuleNodeRes: applyRule Failed: %s with status %i", ruleHead->text, RES_ERR_CODE( statusRes ) );
#endif
        }
    }
    else {
        if ( getNodeType( res ) != N_ERROR && TYPE( res ) != T_BOOL ) {
            char buf[ERR_MSG_LEN];
            generateErrMsg( "error: the rule condition does not evaluate to a boolean value", NODE_EXPR_POS( ruleCondition ), ruleCondition->base, buf );
            addRErrorMsg( errmsg, RE_TYPE_ERROR, buf );
        }
        statusRes = newErrorRes( r, RULE_FAILED_ERR );
    }
    /* copy global variables */
    cpEnv( global, r );
    /* deleteEnv(envNew, 2); */
    region_free( rNew );
    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = RULE_NAME( rule );
        param.ruleIndex = -1;
        reDebug( EXEC_RULE_END, -4, &param, rule, env, rei );
    }

    if ( restoreGlobalREAuditFlag != 0 ) {
        GlobalREAuditFlag = globalREAuditFlag;
    }

    return statusRes;

}

Res* matchPattern( Node *pattern, Node *val, Env *env, ruleExecInfo_t *rei, int reiSaveFlag, rError_t *errmsg, Region *r ) {
    char errbuf[ERR_MSG_LEN];
    char *localErrorMsg = NULL;
    Node *p = pattern, *v = val;
    Res *res;
    char *varName;
    char matcherName[MAX_NAME_LEN];
    RuleIndexListNode *node;

    if ( getNodeType( pattern ) == N_APPLICATION && pattern->subtrees[1]->degree == 0 ) {
        char *fn = pattern->subtrees[0]->text;
        if ( findNextRule2( fn, 0, &node ) == 0 && node->secondaryIndex == 0 ) {
            RuleDesc *rd = getRuleDesc( node->ruleIndex );
            if ( rd->ruleType == RK_FUNC &&
                    ( getNodeType( rd->node->subtrees[2] ) == TK_BOOL ||
                      getNodeType( rd->node->subtrees[2] ) == TK_STRING ||
                      getNodeType( rd->node->subtrees[2] ) == TK_INT ||
                      getNodeType( rd->node->subtrees[2] ) == TK_DOUBLE ) ) {
                pattern = rd->node->subtrees[2];
            }
        }
    }

    switch ( getNodeType( pattern ) ) {
    case N_APPLICATION:
        if ( strcmp( N_APP_FUNC( pattern )->text, "." ) == 0 ) {
            char *key = NULL;
            RE_ERROR2( TYPE( v ) != T_STRING , "not a string." );
            if ( getNodeType( N_APP_ARG( pattern, 1 ) ) == N_APPLICATION && N_APP_ARITY( N_APP_ARG( pattern, 1 ) ) == 0 ) {
                key = N_APP_FUNC( N_APP_ARG( pattern, 1 ) )->text;
            }
            else {
                Res *res = evaluateExpression3( N_APP_ARG( pattern, 1 ), 0, 1, rei, reiSaveFlag, env, errmsg, r );
                if ( res->exprType != NULL && TYPE( res ) == T_STRING ) {
                    key = res->text;
                }
                else {
                    RE_ERROR2( 1, "malformatted key pattern." );
                }
            }
            varName = N_APP_ARG( pattern, 0 )->text;
            if ( getNodeType( N_APP_ARG( pattern, 0 ) ) == TK_VAR && varName[0] == '*' &&
                    ( ( res = ( Res * ) lookupFromEnv( env, varName ) ) == NULL || TYPE( res ) == T_UNSPECED ) ) { /* if local var is empty then create new kvp */
                keyValPair_t *kvp = ( keyValPair_t * ) malloc( sizeof( keyValPair_t ) );
                memset( kvp, 0, sizeof( keyValPair_t ) );
                Res *res2 = newUninterpretedRes( r, KeyValPair_MS_T, kvp, NULL );
                if ( res != NULL ) {
                    updateInEnv( env, varName, res2 );
                }
                else {
                    if ( insertIntoHashTable( env->current, varName, res2 ) == 0 ) {
                        char localErrorMsg[ERR_MSG_LEN];
                        snprintf( localErrorMsg, ERR_MSG_LEN, "error: unable to write to local variable \"%s\".", varName );
                        generateErrMsg( localErrorMsg, NODE_EXPR_POS( N_APP_ARG( pattern, 0 ) ), N_APP_ARG( pattern, 0 )->base, errbuf );
                        addRErrorMsg( errmsg, RE_UNABLE_TO_WRITE_LOCAL_VAR, errbuf );
                        return newErrorRes( r, RE_UNABLE_TO_WRITE_LOCAL_VAR );
                    }
                }

                res = res2;
            }
            else {
                res = evaluateExpression3( N_APP_ARG( pattern, 0 ), 0, 0, rei, reiSaveFlag, env, errmsg, r ); /* kvp */
                CASCADE_N_ERROR( res );
                RE_ERROR2( TYPE( res ) != T_IRODS || strcmp( res->exprType->text, KeyValPair_MS_T ) != 0, "not a KeyValPair." );
            }
            addKeyVal( ( keyValPair_t* ) RES_UNINTER_STRUCT( res ), key, v->text );
            return newIntRes( r, 0 );
        }
        else {
            matcherName[0] = '~';
            strcpy( matcherName + 1, pattern->subtrees[0]->text );
            if ( findNextRule2( matcherName, 0, &node ) == 0 ) {
                v = execRule( matcherName, &val, 1, 0, env, rei, reiSaveFlag, errmsg, r );
                RE_ERROR2( getNodeType( v ) == N_ERROR, "user defined pattern function error" );
                if ( getNodeType( v ) != N_TUPLE ) {
                    Res **tupleComp = ( Res ** )region_alloc( r, sizeof( Res * ) );
                    *tupleComp = v;
                    v = newTupleRes( 1, tupleComp , r );
                }
            }
            else {
                RE_ERROR2( v->text == NULL || strcmp( pattern->subtrees[0]->text, v->text ) != 0, "pattern mismatch constructor" );
                Res **tupleComp = ( Res ** )region_alloc( r, sizeof( Res * ) * v->degree );
                memcpy( tupleComp, v->subtrees, sizeof( Res * ) * v->degree );
                v = newTupleRes( v->degree, tupleComp , r );
            }
            res = matchPattern( p->subtrees[1], v, env, rei, reiSaveFlag, errmsg, r );
            return res;
        }
    case TK_VAR:
        varName = pattern->text;
        if ( varName[0] == '*' ) {
            if ( lookupFromEnv( env, varName ) == NULL ) {
                /* new variable */
                if ( insertIntoHashTable( env->current, varName, val ) == 0 ) {
                    char localErrorMsg[ERR_MSG_LEN];
                    snprintf( localErrorMsg, ERR_MSG_LEN, "error: unable to write to local variable \"%s\".", varName );
                    generateErrMsg( localErrorMsg, NODE_EXPR_POS( pattern ), pattern->base, errbuf );
                    addRErrorMsg( errmsg, RE_UNABLE_TO_WRITE_LOCAL_VAR, errbuf );
                    return newErrorRes( r, RE_UNABLE_TO_WRITE_LOCAL_VAR );
                }
            }
            else {
                updateInEnv( env, varName, val );
            }
        }
        else if ( varName[0] == '$' ) {
            return setVariableValue( varName, val, pattern, rei, env, errmsg, r );
        }
        return newIntRes( r, 0 );

    case N_TUPLE:
        RE_ERROR2( getNodeType( v ) != N_TUPLE, "pattern mismatch value is not a tuple." );
        RE_ERROR2( p->degree != v->degree, "pattern mismatch arity" );
        int i;
        for ( i = 0; i < p->degree; i++ ) {
            Res *res = matchPattern( p->subtrees[i], v->subtrees[i], env, rei, reiSaveFlag, errmsg, r );
            if ( getNodeType( res ) == N_ERROR ) {
                return res;
            }
        }
        return newIntRes( r, 0 );
    case TK_STRING:
        RE_ERROR2( getNodeType( v ) != N_VAL || TYPE( v ) != T_STRING, "pattern mismatch value is not a string." );
        RE_ERROR2( strcmp( pattern->text, v->text ) != 0 , "pattern mismatch string." );
        return newIntRes( r, 0 );
    case TK_BOOL:
        RE_ERROR2( getNodeType( v ) != N_VAL || TYPE( v ) != T_BOOL, "pattern mismatch value is not a boolean." );
        res = evaluateExpression3( pattern, 0, 1, rei, reiSaveFlag, env, errmsg, r );
        CASCADE_N_ERROR( res );
        RE_ERROR2( RES_BOOL_VAL( res ) != RES_BOOL_VAL( v ) , "pattern mismatch boolean." );
        return newIntRes( r, 0 );
    case TK_INT:
        RE_ERROR2( getNodeType( v ) != N_VAL || ( TYPE( v ) != T_INT && TYPE( v ) != T_DOUBLE ), "pattern mismatch value is not an integer." );
        res = evaluateExpression3( pattern, 0, 1, rei, reiSaveFlag, env, errmsg, r );
        CASCADE_N_ERROR( res );
        RE_ERROR2( RES_INT_VAL( res ) != ( TYPE( v ) == T_INT ? RES_INT_VAL( v ) : RES_DOUBLE_VAL( v ) ) , "pattern mismatch integer." );
        return newIntRes( r, 0 );
    case TK_DOUBLE:
        RE_ERROR2( getNodeType( v ) != N_VAL || ( TYPE( v ) != T_DOUBLE && TYPE( v ) != T_INT ), "pattern mismatch value is not a double." );
        res = evaluateExpression3( pattern, 0, 1, rei, reiSaveFlag, env, errmsg, r );
        CASCADE_N_ERROR( res );
        RE_ERROR2( RES_DOUBLE_VAL( res ) != ( TYPE( v ) == T_DOUBLE ? RES_DOUBLE_VAL( v ) : RES_INT_VAL( v ) ), "pattern mismatch integer." );
        return newIntRes( r, 0 );
    default:
        RE_ERROR2( 1, "malformatted pattern error" );
        break;
    }
error:
    generateErrMsg( localErrorMsg, NODE_EXPR_POS( pattern ), pattern->base, errbuf );
    addRErrorMsg( errmsg, RE_PATTERN_NOT_MATCHED, errbuf );
    return newErrorRes( r, RE_PATTERN_NOT_MATCHED );

}

Res *setVariableValue( char *varName, Res *val, Node *node, ruleExecInfo_t *rei, Env *env, rError_t *errmsg, Region *r ) {
    int i;
    char *varMap;
    char errbuf[ERR_MSG_LEN];
    if ( varName[0] == '$' ) {
        const char *arg = varName + 1;
        if ( ( i = applyRuleArg( "acPreProcForWriteSessionVariable", &arg, 1, rei, 0 ) ) < 0 ) {
            return newErrorRes( r, i );
        }
        i = getVarMap( "", varName, &varMap, 0 );
        if ( i < 0 ) {
            snprintf( errbuf, ERR_MSG_LEN, "error: unsupported session variable \"%s\".", varName );
            addRErrorMsg( errmsg, RE_UNSUPPORTED_SESSION_VAR, errbuf );
            free( varMap );
            return newErrorRes( r, RE_UNSUPPORTED_SESSION_VAR );
        }
        FunctionDesc *fd = ( FunctionDesc * ) lookupFromEnv( ruleEngineConfig.extFuncDescIndex, varMap );
        Hashtable *tvarEnv = newHashTable2( 10, r );
        if ( fd != NULL ) {
            ExprType *type = fd->exprType->subtrees[0]; /* get var type from varMap */
            val = processCoercion( node, val, type, tvarEnv, errmsg, r );
            if ( getNodeType( val ) == N_ERROR ) {
                free( varMap );
                return val;
            }
        }
        ExprType *varType = getVarType( varMap, r );
        val = processCoercion( node, val, varType, tvarEnv, errmsg, r );
        if ( getNodeType( val ) == N_ERROR ) {
            free( varMap );
            return val;
        }
        setVarValue( varMap, rei, val );
        free( varMap );
        return newIntRes( r, 0 );
    }
    else if ( varName[0] == '*' ) {
        if ( lookupFromEnv( env, varName ) == NULL ) {
            /* new variable */
            if ( insertIntoHashTable( env->current, varName, val ) == 0 ) {
                snprintf( errbuf, ERR_MSG_LEN, "error: unable to write to local variable \"%s\".", varName );
                addRErrorMsg( errmsg, RE_UNABLE_TO_WRITE_LOCAL_VAR, errbuf );
                return newErrorRes( r, RE_UNABLE_TO_WRITE_LOCAL_VAR );
            }
        }
        else {
            updateInEnv( env, varName, val );
        }
        return newIntRes( r, 0 );
    }
    return newIntRes( r, 0 );
}

int definitelyEq( Res *a, Res *b ) {
    if ( a != b && TYPE( a ) == TYPE( b ) ) {
        switch ( TYPE( a ) ) {
        case T_INT:
            return RES_INT_VAL( a ) == RES_INT_VAL( b );
        case T_DOUBLE:
            return RES_DOUBLE_VAL( a ) == RES_DOUBLE_VAL( b );
        case T_STRING:
            return strcmp( a->text, b->text ) == 0 ? 1 : 0;
        case T_DATETIME:
            return RES_TIME_VAL( a ) == RES_TIME_VAL( b );
        case T_BOOL:
            return RES_BOOL_VAL( a ) == RES_BOOL_VAL( b );
        case T_IRODS:
            return RES_UNINTER_STRUCT( a ) == RES_UNINTER_STRUCT( b ) && RES_UNINTER_BUFFER( a ) == RES_UNINTER_BUFFER( b );
        case T_PATH:
            return strcmp( a->text, b->text ) == 0 ? 1 : 0;
        case T_CONS:
            if ( a->degree == b->degree ) {
                if ( a->text == b->text || strcmp( a->text, b->text ) == 0 ) {
                    int res = 1;
                    for ( int i = 0; i < a->degree; i++ ) {
                        if ( !definitelyEq( a->subtrees[i], b->subtrees[i] ) ) {
                            res = 0;
                            break;
                        }
                    }
                    return res;
                }
            }
            return 0;
        case T_TUPLE:
            if ( a->degree == b->degree ) {
                int res = 1;
                for ( int i = 0; i < a->degree; i++ ) {
                    if ( !definitelyEq( a->subtrees[i], b->subtrees[i] ) ) {
                        res = 0;
                        break;
                    }
                }
                return res;
            }
            return 0;
        default:
            return 0;
        }

    }
    return a == b;
}

