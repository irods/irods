/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* #define RE_LOG_RULES_TMP */
#define HAS_MICROSDEF_T
#include "rodsConnect.h"
#include "reGlobals.hpp"
#include "reFuncDefs.hpp"
#include "reHelpers1.hpp"
#include "reAction.hpp"
#include "apiHeaderAll.hpp"
#include "parser.hpp"
#include "index.hpp"
#include "rules.hpp"
#include "cache.hpp"
#include "locks.hpp"
#include "functions.hpp"
#include "configuration.hpp"

#include "irods_get_full_path_for_config_file.hpp"

#ifdef MYMALLOC
# Within reLib1.c here, change back the redefines of malloc back to normal
#define malloc(x) malloc(x)
#define free(x) free(x)
#endif

int processReturnRes( Res *res );

int
applyRuleArg( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
              ruleExecInfo_t *rei, int reiSaveFlag ) {
#ifdef DEBUG
    writeToTmp( "entry.log", "applyRuleArg: " );
    writeToTmp( "entry.log", action );
    writeToTmp( "entry.log", "(pass on to applyRulePA)\n" );
#endif
    msParamArray_t *inMsParamArray = NULL;
    int i;

    i = applyRuleArgPA( action , args,  argc, inMsParamArray, rei, reiSaveFlag );
    return i;
}

/*
 * devnote: this method differs from applyRuleArg in that it allows passing in a preallocated msParamArray_t
 */

int
applyRuleArgPA( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
                msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ) {
#ifdef DEBUG
    writeToTmp( "entry.log", "applyRuleArgPa: " );
    writeToTmp( "entry.log", action );
    writeToTmp( "entry.log", "\n" );
#endif
    int i;

    Region *r = make_region( 0, NULL );
    rError_t errmsgBuf;
    errmsgBuf.errMsg = NULL;
    errmsgBuf.len = 0;
    Res *res = computeExpressionWithParams( action, args, argc, rei, reiSaveFlag, inMsParamArray, &errmsgBuf, r );
    i = processReturnRes( res );
    region_free( r );
    /* applyRule(tmpStr, inMsParamArray, rei, reiSaveFlag); */
    if ( i < 0 ) {
        logErrMsg( &errmsgBuf, &rei->rsComm->rError );
    }
    freeRErrorContent( &errmsgBuf );
    return i;

}

/* utility function */
int processReturnRes( Res *res ) {
    int ret;
    if ( res->nodeType == N_ERROR ) {
        ret = RES_ERR_CODE( res );
    }
    else {
        switch ( TYPE( res ) ) {
        case T_INT:
            ret = RES_INT_VAL( res );
            break;
        default:
            ret = 0; /* other types */
            break;
        }
    }

    return ret;
}
int computeExpression( char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag, char *res ) {
#ifdef DEBUG
    writeToTmp( "entry.log", "computeExpression: " );
    writeToTmp( "entry.log", inAction );
    writeToTmp( "entry.log", "\n" );
#endif

    Region *r = make_region( 0, NULL );

    Res *res0 = parseAndComputeExpressionAdapter( inAction, inMsParamArray, 0, rei, reiSaveFlag, r );
    int ret;
    char *res1 = convertResToString( res0 );
    snprintf( res, MAX_COND_LEN, "%s", res1 );
    free( res1 );

    if ( getNodeType( res0 ) == N_ERROR ) {
        ret = RES_ERR_CODE( res0 );
    }
    else {
        switch ( TYPE( res0 ) ) {
        case T_INT:
            ret = RES_INT_VAL( res0 );
            break;
        case T_BOOL:
            ret = !RES_BOOL_VAL( res0 );
            break;
        default:
            ret = 0; /* other types */
            break;
        }
    }
    region_free( r );

    return ret;
}


/* This function computes the expression in inAction using inMsParamArray as the env.
 * It doesn't update inMsParamArray with the new env. */
int
applyRule( char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ) {
    return applyRuleBase( inAction, inMsParamArray, 0, rei, reiSaveFlag );
}

/* This function computes the expression in inAction using inMsParamArray as the env and updates inMsParamArray with the new env. */
int
applyRuleUpdateParams( char *inAction, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag ) {
    return applyRuleBase( inAction, inMsParamArray, 1, rei, reiSaveFlag );

}

int
applyRuleBase( char *inAction, msParamArray_t *inMsParamArray, int updateInMsParam, ruleExecInfo_t *rei, int reiSaveFlag ) {
#if defined(DEBUG) || defined(RE_LOG_RULES_TMP)
    writeToTmp( "entry.log", "applyRule: " );
    writeToTmp( "entry.log", inAction );
    writeToTmp( "entry.log", "\n" );
#endif
    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = inAction;
        param.ruleIndex = -1;
        reDebug( APPLY_RULE_BEGIN, -1, &param, NULL, NULL, rei );

    }

    Region *r = make_region( 0, NULL );

    int ret;
    Res *res;
    if ( inAction[strlen( inAction ) - 1] == '|' ) {
        char *inActionCopy = strdup( inAction );
        inActionCopy[strlen( inAction ) - 1] = '\0';
        char *action = ( char * ) malloc( sizeof( char ) * strlen( inAction ) + 3 );
        sprintf( action, "{%s}", inActionCopy );
        res = parseAndComputeExpressionAdapter( action, inMsParamArray, updateInMsParam, rei, reiSaveFlag, r );
        free( action );
        free( inActionCopy );
    }
    else {
        res = parseAndComputeExpressionAdapter( inAction, inMsParamArray, updateInMsParam, rei, reiSaveFlag, r );
    }
    ret = processReturnRes( res );
    region_free( r );
    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = inAction;
        param.ruleIndex = -1;
        reDebug( APPLY_RULE_END, -1, &param, NULL, NULL, rei );
    }

    return ret;

}





int
applyAllRules( char *inAction, msParamArray_t *inMsParamArray,
               ruleExecInfo_t *rei, int reiSaveFlag, int allRuleExecFlag ) {
#ifdef DEBUG
    writeToTmp( "entry.log", "applyAllRules: " );
    writeToTmp( "entry.log", inAction );
    writeToTmp( "entry.log", "(set GlobalAllRuleExecFlag and forward to applyRule)\n" );
#endif

    /* store global flag in a temp variables */
    int tempFlag = GlobalAllRuleExecFlag;
    /* set global flag */
    GlobalAllRuleExecFlag = allRuleExecFlag == 1 ? 2 : 1;

    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = inAction;
        param.ruleIndex = -1;
        reDebug( APPLY_ALL_RULES_BEGIN, -1, &param, NULL, NULL, rei );
    }

    int ret = applyRule( inAction, inMsParamArray, rei, reiSaveFlag );

    if ( GlobalREAuditFlag > 0 ) {
        RuleEngineEventParam param;
        param.actionName = inAction;
        param.ruleIndex = -1;
        reDebug( APPLY_ALL_RULES_END, -1, &param, NULL, NULL, rei );
    }

    /* restore global flag */
    GlobalAllRuleExecFlag = tempFlag;
    return ret;
}

int
execMyRule( char * ruleDef, msParamArray_t *inMsParamArray, char *outParamsDesc,
            ruleExecInfo_t *rei ) {

    return execMyRuleWithSaveFlag( ruleDef, inMsParamArray, outParamsDesc, rei, 0 );
}
void appendOutputToInput( msParamArray_t *inpParamArray, char **outParamNames, int outParamN ) {
    int i, k, repeat = 0;
    for ( i = 0; i < outParamN; i++ ) {
        if ( strcmp( outParamNames[i], ALL_MS_PARAM_KW ) == 0 ) {
            continue;
        }
        repeat = 0;
        for ( k = 0; k < inpParamArray->len; k++ ) {
            if ( inpParamArray->msParam[k]->label != NULL && strcmp( outParamNames[i], inpParamArray->msParam[k]->label ) == 0 ) {
                repeat = 1;
                break;
            }
        }
        if ( !repeat ) {
            addMsParam( inpParamArray, outParamNames[i], NULL, NULL, NULL );
        }
    }

}
int extractVarNames( char **varNames, char *outBuf ) {
    int n = 0;
    char *p = outBuf;
    char *psrc = p;

    for ( ;; ) {
        if ( *psrc == '%' ) {
            *psrc = '\0';
            varNames[n++] = strdup( p );
            *psrc = '%';
            p = psrc + 1;
        }
        else if ( *psrc == '\0' ) {
            if ( strlen( p ) != 0 ) {
                varNames[n++] = strdup( p );
            }
            break;
        }
        psrc++;
    }
    return n;
}

int
execMyRuleWithSaveFlag( char * ruleDef, msParamArray_t *inMsParamArray, char *outParamsDesc,
                        ruleExecInfo_t *rei, int reiSaveFlag )

{
    int status;
    if ( GlobalREAuditFlag ) {
        RuleEngineEventParam param;
        param.actionName = ruleDef;
        param.ruleIndex = -1;
        reDebug( EXEC_MY_RULE_BEGIN, -1, &param, NULL, NULL, rei );
    }


    char *outParamNames[MAX_PARAMS_LEN];
    int n = extractVarNames( outParamNames, outParamsDesc );
    appendOutputToInput( inMsParamArray, outParamNames, n );

    int i;
    for ( i = 0; i < n; i++ ) {
        free( outParamNames[i] );
    }

    Region *r = make_region( 0, NULL );
    status =
        parseAndComputeRuleAdapter( ruleDef, inMsParamArray, rei, reiSaveFlag, r );
    region_free( r );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "execMyRule %s Failed with status %i", ruleDef, status );
    }

    if ( GlobalREAuditFlag ) {
        RuleEngineEventParam param;
        param.actionName = ruleDef;
        param.ruleIndex = -1;
        reDebug( EXEC_MY_RULE_END, -1, &param, NULL, NULL, rei );
    }
    return status;
}

int
initRuleStruct( int processType, rsComm_t *svrComm, char *irbSet, char *dvmSet, char *fnmSet ) {
    int i;
    char r1[NAME_LEN], r2[RULE_SET_DEF_LENGTH], r3[RULE_SET_DEF_LENGTH];

    /*strcpy(r2,irbSet);*/
    coreRuleStrct.MaxNumOfRules = 0;
    appRuleStrct.MaxNumOfRules = 0;
    GlobalAllRuleExecFlag = 0;

    if ( processType == RULE_ENGINE_INIT_CACHE ) {
        resetMutex();
    }
    /*while (strlen(r2) > 0) {
      i = rSplitStr(r2,r1,NAME_LEN,r3,RULE_SET_DEF_LENGTH,',');
      if (i == 0)*/
    i = readRuleStructFromFile( processType, irbSet, &coreRuleStrct );
    if ( i < 0 ) {
        return i;
    }
    /*strcpy(r2,r3);
    }*/
    snprintf( r2, sizeof( r2 ), "%s", dvmSet );
    coreRuleVarDef.MaxNumOfDVars = 0;
    appRuleVarDef.MaxNumOfDVars = 0;

    while ( strlen( r2 ) > 0 ) {
        i = rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        if ( i == 0 ) {
            i = readDVarStructFromFile( r1, &coreRuleVarDef );
        }
        if ( i < 0 ) {
            return i;
        }
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }
    snprintf( r2, sizeof( r2 ), "%s", fnmSet );
    coreRuleFuncMapDef.MaxNumOfFMaps = 0;
    appRuleFuncMapDef.MaxNumOfFMaps = 0;

    while ( strlen( r2 ) > 0 ) {
        i = rSplitStr( r2, r1, NAME_LEN, r3, RULE_SET_DEF_LENGTH, ',' );
        if ( i == 0 ) {
            i = readFuncMapStructFromFile( r1, &coreRuleFuncMapDef );
        }
        if ( i < 0 ) {
            return i;
        }
        snprintf( r2, sizeof( r2 ), "%s", r3 );
    }

    /* read logging settings */
    if ( svrComm != NULL ) { /* if this is not a process started by a client, then we used the default logging setting */
        readICatUserLogging( svrComm->clientUser.userName, &ruleEngineConfig.logging, svrComm );
    }

    if ( getenv( RETESTFLAG ) != NULL ) {
        reTestFlag = atoi( getenv( RETESTFLAG ) );
        if ( getenv( RELOOPBACKFLAG ) != NULL ) {
            reLoopBackFlag = atoi( getenv( RELOOPBACKFLAG ) );
        }
        else {
            reLoopBackFlag = 0;
        }
    }
    else {
        reTestFlag = 0;
        reLoopBackFlag = 0;
    }
    if ( getenv( "GLOBALALLRULEEXECFLAG" ) != NULL ) {
        GlobalAllRuleExecFlag = 9;
    }

    if ( getenv( GLOBALREDEBUGFLAG ) != NULL ) {
        GlobalREDebugFlag = atoi( getenv( GLOBALREDEBUGFLAG ) );
    }
    if ( getenv( GLOBALREAUDITFLAG ) != NULL ) {
        GlobalREAuditFlag = atoi( getenv( GLOBALREAUDITFLAG ) );
    }
    if ( GlobalREAuditFlag == 0 ) {
        GlobalREAuditFlag = GlobalREDebugFlag;
    }
    delayStack.size = NAME_LEN;
    delayStack.len = 0;
    delayStack.value = NULL;

    msParamStack.size = NAME_LEN;
    msParamStack.len = 0;
    msParamStack.value = NULL;

    initializeReDebug( svrComm );

    return 0;
}


int
readRuleSetFromDB( char *ruleBaseName, char *versionStr, RuleSet *ruleSet, ruleExecInfo_t *rei, rError_t *errmsg, Region *region ) {
    int i, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
    sqlResult_t *r[8];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condstr, MAX_NAME_LEN, "= '%s'", ruleBaseName );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_BASE_NAME, condstr );
    snprintf( condstr2, MAX_NAME_LEN, "= '%s'", versionStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_VERSION, condstr2 );

    addInxIval( &genQueryInp.selectInp, COL_RULE_BASE_MAP_PRIORITY, ORDER_BY );
    addInxIval( &genQueryInp.selectInp, COL_RULE_BASE_MAP_BASE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EVENT, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_CONDITION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_BODY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_RECOVERY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_ID, 1 );
    Env *env = newEnv( newHashTable2( 100, region ), NULL, NULL, region );
    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
        r[0] = getSqlResultByInx( genQueryOut, COL_RULE_BASE_MAP_BASE_NAME );
        r[1] = getSqlResultByInx( genQueryOut, COL_RULE_NAME );
        r[2] = getSqlResultByInx( genQueryOut, COL_RULE_EVENT );
        r[3] = getSqlResultByInx( genQueryOut, COL_RULE_CONDITION );
        r[4] = getSqlResultByInx( genQueryOut, COL_RULE_BODY );
        r[5] = getSqlResultByInx( genQueryOut, COL_RULE_RECOVERY );
        r[6] = getSqlResultByInx( genQueryOut, COL_RULE_ID );
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            char ruleStr[MAX_RULE_LEN * 4];
            /* char *ruleBase = strdup(&r[0]->value[r[0]->len * i]);
            char *action   = strdup(&r[1]->value[r[1]->len * i]); */
            char *ruleHead = strdup( &r[2]->value[r[2]->len * i] );
            char *ruleCondition = strdup( &r[3]->value[r[3]->len * i] );
            char *ruleAction    = strdup( &r[4]->value[r[4]->len * i] );
            char *ruleRecovery  = strdup( &r[5]->value[r[5]->len * i] );
            long ruleId = atol( &r[6]->value[r[6]->len * i] );
            if ( ruleRecovery[0] == '@' ) {
                /* rulegen */
                if ( strcmp( ruleRecovery + 1, "DATA" ) == 0 ) {
                    snprintf( ruleStr, MAX_RULE_LEN * 4, "data %s", ruleHead );
                }
                else if ( strcmp( ruleRecovery + 1, "CONSTR" ) == 0 ) {
                    snprintf( ruleStr, MAX_RULE_LEN * 4, "constructor %s : %s", ruleHead, ruleAction );
                }
                else if ( strcmp( ruleRecovery + 1, "EXTERN" ) == 0 ) {
                    snprintf( ruleStr, MAX_RULE_LEN * 4, "%s : %s", ruleHead, ruleAction );
                }
                else if ( strcmp( ruleRecovery + 1, "FUNC" ) == 0 ) {
                    snprintf( ruleStr, MAX_RULE_LEN * 4, "%s = %s\n @(\"id\", \"%ld\")", ruleHead, ruleAction, ruleId );
                }
                else {
                    snprintf( ruleStr, MAX_RULE_LEN * 4, "%s { on %s %s @(\"id\", \"%ld\") }\n", ruleHead, ruleCondition, ruleAction, ruleId );
                }
            }
            else {
                snprintf( ruleStr, MAX_RULE_LEN * 4, "%s|%s|%s|%s", ruleHead, ruleCondition, ruleAction, ruleRecovery );
            }
            Pointer *p = newPointer2( ruleStr );
            int errloc;
            int errcode = parseRuleSet( p, ruleSet, env, &errloc, errmsg, region );
            deletePointer( p );
            if ( errcode < 0 ) {
                /* deleteEnv(env, 3); */
                freeGenQueryOut( &genQueryOut );
                free( ruleHead ); // cppcheck - Memory leak: ruleHead
                free( ruleCondition ); // cppcheck - Memory leak: ruleCondition
                free( ruleAction ); // cppcheck - Memory leak: ruleAction
                free( ruleRecovery ); // cppcheck - Memory leak: ruleRecovery
                return errcode;
            }
            free( ruleHead ); // cppcheck - Memory leak: ruleHead
            free( ruleCondition ); // cppcheck - Memory leak: ruleCondition
            free( ruleAction ); // cppcheck - Memory leak: ruleAction
            free( ruleRecovery ); // cppcheck - Memory leak: ruleRecovery
        }
        genQueryInp.continueInx =  genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
        if ( genQueryInp.continueInx  > 0 ) {
            /* More to come */
            status =  rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }
    /* deleteEnv(env, 3); */
    return 0;
}

int
readRuleStructFromDB( char *ruleBaseName, char *versionStr, ruleStruct_t *inRuleStrct, ruleExecInfo_t *rei ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
    sqlResult_t *r[8];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condstr, MAX_NAME_LEN, "= '%s'", ruleBaseName );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_BASE_NAME, condstr );
    snprintf( condstr2, MAX_NAME_LEN, "= '%s'", versionStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_BASE_MAP_VERSION, condstr2 );

    addInxIval( &genQueryInp.selectInp, COL_RULE_BASE_MAP_PRIORITY, ORDER_BY );
    addInxIval( &genQueryInp.selectInp, COL_RULE_BASE_MAP_BASE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EVENT, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_CONDITION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_BODY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_RECOVERY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_ID, 1 );
    int continueInx = 1;
    while ( continueInx > 0 && rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut ) >= 0 && genQueryOut->rowCnt > 0 ) {
        r[0] = getSqlResultByInx( genQueryOut, COL_RULE_BASE_MAP_BASE_NAME );
        r[1] = getSqlResultByInx( genQueryOut, COL_RULE_NAME );
        r[2] = getSqlResultByInx( genQueryOut, COL_RULE_EVENT );
        r[3] = getSqlResultByInx( genQueryOut, COL_RULE_CONDITION );
        r[4] = getSqlResultByInx( genQueryOut, COL_RULE_BODY );
        r[5] = getSqlResultByInx( genQueryOut, COL_RULE_RECOVERY );
        r[6] = getSqlResultByInx( genQueryOut, COL_RULE_ID );
        for ( int i = 0; i < genQueryOut->rowCnt; i++ ) {
            inRuleStrct->ruleBase[inRuleStrct->MaxNumOfRules] = strdup( &r[0]->value[r[0]->len * i] );
            inRuleStrct->action[inRuleStrct->MaxNumOfRules]   = strdup( &r[1]->value[r[1]->len * i] );
            inRuleStrct->ruleHead[inRuleStrct->MaxNumOfRules] = strdup( &r[2]->value[r[2]->len * i] );
            inRuleStrct->ruleCondition[inRuleStrct->MaxNumOfRules] = strdup( &r[3]->value[r[3]->len * i] );
            inRuleStrct->ruleAction[inRuleStrct->MaxNumOfRules]    = strdup( &r[4]->value[r[4]->len * i] );
            inRuleStrct->ruleRecovery[inRuleStrct->MaxNumOfRules]  = strdup( &r[5]->value[r[5]->len * i] );
            inRuleStrct->ruleId[inRuleStrct->MaxNumOfRules] = atol( &r[6]->value[r[6]->len * i] );
            inRuleStrct->MaxNumOfRules++;
        }
        continueInx = genQueryInp.continueInx =  genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
    }
    return 0;
}


int
readDVMapStructFromDB( char *dvmBaseName, char *versionStr, rulevardef_t *inDvmStrct, ruleExecInfo_t *rei ) {
    int i, l, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
    sqlResult_t *r[5];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condstr, MAX_NAME_LEN, "= '%s'", dvmBaseName );
    addInxVal( &genQueryInp.sqlCondInp, COL_DVM_BASE_MAP_BASE_NAME, condstr );
    snprintf( condstr2, MAX_NAME_LEN, "= '%s'", versionStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_DVM_BASE_MAP_VERSION, condstr2 );

    addInxIval( &genQueryInp.selectInp, COL_DVM_EXT_VAR_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DVM_CONDITION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DVM_INT_MAP_PATH, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DVM_ID, ORDER_BY );
    l = inDvmStrct->MaxNumOfDVars;
    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
        r[0] = getSqlResultByInx( genQueryOut, COL_DVM_EXT_VAR_NAME );
        r[1] = getSqlResultByInx( genQueryOut, COL_DVM_CONDITION );
        r[2] = getSqlResultByInx( genQueryOut, COL_DVM_INT_MAP_PATH );
        r[3] = getSqlResultByInx( genQueryOut, COL_DVM_ID );
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            inDvmStrct->varName[l]   = strdup( &r[0]->value[r[0]->len * i] );
            inDvmStrct->action[l] = strdup( &r[1]->value[r[1]->len * i] );
            inDvmStrct->var2CMap[l] = strdup( &r[2]->value[r[2]->len * i] );
            inDvmStrct->varId[l] = atol( &r[3]->value[r[3]->len * i] );
            l++;
        }
        genQueryInp.continueInx =  genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
        if ( genQueryInp.continueInx  > 0 ) {
            /* More to come */
            status =  rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }
    inDvmStrct->MaxNumOfDVars = l;
    return 0;
}



int
readFNMapStructFromDB( char *fnmBaseName, char *versionStr, fnmapStruct_t *inFnmStrct, ruleExecInfo_t *rei ) {
    int i, l, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condstr[MAX_NAME_LEN], condstr2[MAX_NAME_LEN];
    sqlResult_t *r[5];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    snprintf( condstr, MAX_NAME_LEN, "= '%s'", fnmBaseName );
    addInxVal( &genQueryInp.sqlCondInp, COL_FNM_BASE_MAP_BASE_NAME, condstr );
    snprintf( condstr2, MAX_NAME_LEN, "= '%s'", versionStr );
    addInxVal( &genQueryInp.sqlCondInp, COL_FNM_BASE_MAP_VERSION, condstr2 );

    addInxIval( &genQueryInp.selectInp, COL_FNM_EXT_FUNC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_FNM_INT_FUNC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_FNM_ID, ORDER_BY );

    l = inFnmStrct->MaxNumOfFMaps;
    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
        r[0] = getSqlResultByInx( genQueryOut, COL_FNM_EXT_FUNC_NAME );
        r[1] = getSqlResultByInx( genQueryOut, COL_FNM_INT_FUNC_NAME );
        r[2] = getSqlResultByInx( genQueryOut, COL_FNM_ID );
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            inFnmStrct->funcName[l]   = strdup( &r[0]->value[r[0]->len * i] );
            inFnmStrct->func2CMap[l] = strdup( &r[1]->value[r[1]->len * i] );
            inFnmStrct->fmapId[l] = atol( &r[2]->value[r[2]->len * i] );
            l++;
        }
        genQueryInp.continueInx =  genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
        if ( genQueryInp.continueInx  > 0 ) {
            /* More to come */
            status =  rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }
    inFnmStrct->MaxNumOfFMaps = l;
    return 0;
}



int
readMsrvcStructFromDB( int inStatus, msrvcStruct_t *inMsrvcStrct, ruleExecInfo_t *rei ) {
    int i, l, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    sqlResult_t *r[10];
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    genQueryInp.maxRows = MAX_SQL_ROWS;
    char condstr[MAX_NAME_LEN];

    snprintf( condstr, MAX_NAME_LEN, "= '%i'", inStatus );
    addInxVal( &genQueryInp.sqlCondInp, COL_MSRVC_STATUS, condstr );

    addInxIval( &genQueryInp.selectInp, COL_MSRVC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_MODULE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_SIGNATURE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_VERSION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_HOST, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_LOCATION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_LANGUAGE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_TYPE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_STATUS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_MSRVC_ID, ORDER_BY );

    l = inMsrvcStrct->MaxNumOfMsrvcs;
    status = rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
    while ( status >= 0 && genQueryOut->rowCnt > 0 ) {
        r[0] = getSqlResultByInx( genQueryOut, COL_MSRVC_MODULE_NAME );
        r[1] = getSqlResultByInx( genQueryOut, COL_MSRVC_NAME );
        r[2] = getSqlResultByInx( genQueryOut, COL_MSRVC_SIGNATURE );
        r[3] = getSqlResultByInx( genQueryOut, COL_MSRVC_VERSION );
        r[4] = getSqlResultByInx( genQueryOut, COL_MSRVC_HOST );
        r[5] = getSqlResultByInx( genQueryOut, COL_MSRVC_LOCATION );
        r[6] = getSqlResultByInx( genQueryOut, COL_MSRVC_LANGUAGE );
        r[7] = getSqlResultByInx( genQueryOut, COL_MSRVC_TYPE_NAME );
        r[8] = getSqlResultByInx( genQueryOut, COL_MSRVC_STATUS );
        r[9] = getSqlResultByInx( genQueryOut, COL_MSRVC_ID );
        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            inMsrvcStrct->moduleName[l] = strdup( &r[0]->value[r[0]->len * i] );
            inMsrvcStrct->msrvcName[l]   = strdup( &r[1]->value[r[1]->len * i] );
            inMsrvcStrct->msrvcSignature[l] = strdup( &r[2]->value[r[2]->len * i] );
            inMsrvcStrct->msrvcVersion[l] = strdup( &r[3]->value[r[3]->len * i] );
            inMsrvcStrct->msrvcHost[l]    = strdup( &r[4]->value[r[4]->len * i] );
            inMsrvcStrct->msrvcLocation[l]  = strdup( &r[5]->value[r[5]->len * i] );
            inMsrvcStrct->msrvcLanguage[l]  = strdup( &r[6]->value[r[6]->len * i] );
            inMsrvcStrct->msrvcTypeName[l]  = strdup( &r[7]->value[r[7]->len * i] );
            inMsrvcStrct->msrvcStatus[l]  = atol( &r[8]->value[r[8]->len * i] );
            inMsrvcStrct->msrvcId[l]  = atol( &r[9]->value[r[9]->len * i] );
            l++;
        }
        genQueryInp.continueInx =  genQueryOut->continueInx;
        freeGenQueryOut( &genQueryOut );
        if ( genQueryInp.continueInx  > 0 ) {
            /* More to come */
            status =  rsGenQuery( rei->rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }
    inMsrvcStrct->MaxNumOfMsrvcs = l;
    return 0;
}

int
readRuleStructFromFile( int processType, char *ruleBaseName, ruleStruct_t *inRuleStrct ) {

    return loadRuleFromCacheOrFile( processType, ruleBaseName, inRuleStrct );
}

int
clearRuleStruct( ruleStruct_t *inRuleStrct ) {
    int i;
    for ( i = 0 ; i < inRuleStrct->MaxNumOfRules ; i++ ) {
        if ( inRuleStrct->ruleBase[i]  != NULL ) {
            free( inRuleStrct->ruleBase[i] );
        }
        if ( inRuleStrct->ruleHead[i]  != NULL ) {
            free( inRuleStrct->ruleHead[i] );
        }
        if ( inRuleStrct->ruleCondition[i]  != NULL ) {
            free( inRuleStrct->ruleCondition[i] );
        }
        if ( inRuleStrct->ruleAction[i]  != NULL ) {
            free( inRuleStrct->ruleAction[i] );
        }
        if ( inRuleStrct->ruleRecovery[i]  != NULL ) {
            free( inRuleStrct->ruleRecovery[i] );
        }

    }
    inRuleStrct->MaxNumOfRules  = 0;
    if ( inRuleStrct == &coreRuleStrct ) {
        clearResources( RESC_CORE_RULE_SET | RESC_CORE_FUNC_DESC_INDEX );
    }
    else if ( inRuleStrct == &appRuleStrct ) {
        clearResources( RESC_APP_RULE_SET | RESC_APP_FUNC_DESC_INDEX );
    }

    return 0;
}

int clearDVarStruct( rulevardef_t *inRuleVarDef ) {
    int i;
    for ( i = 0 ; i < inRuleVarDef->MaxNumOfDVars; i++ ) {
        if ( inRuleVarDef->varName[i] != NULL ) {
            free( inRuleVarDef->varName[i] );
        }
        if ( inRuleVarDef->action[i] != NULL ) {
            free( inRuleVarDef->action[i] );
        }
        if ( inRuleVarDef->var2CMap[i] != NULL ) {
            free( inRuleVarDef->var2CMap[i] );
        }
    }
    inRuleVarDef->MaxNumOfDVars =  0;
    return 0;
}

int clearFuncMapStruct( rulefmapdef_t* inRuleFuncMapDef ) {
    int i;
    for ( i = 0 ; i < inRuleFuncMapDef->MaxNumOfFMaps; i++ ) {
        if ( inRuleFuncMapDef->funcName[i] != NULL ) {
            free( inRuleFuncMapDef->funcName[i] );
        }
        if ( inRuleFuncMapDef->func2CMap[i] != NULL ) {
            free( inRuleFuncMapDef->func2CMap[i] );
        }
    }
    inRuleFuncMapDef->MaxNumOfFMaps = 0;
    if ( inRuleFuncMapDef == &coreRuleFuncMapDef ) {
        clearIndex( &coreRuleFuncMapDefIndex );
    }
    else if ( inRuleFuncMapDef == &appRuleFuncMapDef ) {
        clearIndex( &appRuleFuncMapDefIndex );
    }

    return 0;
}


int
readDVarStructFromFile( char *dvarBaseName, rulevardef_t *inRuleVarDef ) {
    int i = 0;
    char l0[MAX_DVAR_LENGTH];
    char l1[MAX_DVAR_LENGTH];
    char l2[MAX_DVAR_LENGTH];
    char l3[MAX_DVAR_LENGTH];
    char dvarsFileName[MAX_NAME_LEN];
    FILE *file;
    char buf[MAX_DVAR_LENGTH];
    //char *configDir;

    i = inRuleVarDef->MaxNumOfDVars;

    if ( dvarBaseName[0] == '/' || dvarBaseName[0] == '\\' ||
            dvarBaseName[1] == ':' ) {
        snprintf( dvarsFileName, MAX_NAME_LEN, "%s", dvarBaseName );
    }
    else {
        std::string cfg_file, fn( dvarBaseName );
        fn += ".dvm";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( dvarsFileName, sizeof( dvarsFileName ), "%s", cfg_file.c_str() );

    }
    file = fopen( dvarsFileName, "r" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "readDvarStructFromFile() could not open dvm file %s\n",
                 dvarsFileName );
        return DVARMAP_FILE_READ_ERROR;
    }
    buf[MAX_DVAR_LENGTH - 1] = '\0';
    while ( fgets( buf, MAX_DVAR_LENGTH - 1, file ) != NULL ) {
        if ( buf[strlen( buf ) - 1] == '\n' ) {
            buf[strlen( buf ) - 1] = '\0';
        }
        if ( buf[0] == '#' || strlen( buf ) < 4 ) {
            continue;
        }
        rSplitStr( buf, l1, MAX_DVAR_LENGTH, l0, MAX_DVAR_LENGTH, '|' );
        inRuleVarDef->varName[i] = strdup( l1 ); /** varName **/
        rSplitStr( l0, l1, MAX_DVAR_LENGTH, l3, MAX_DVAR_LENGTH, '|' );
        inRuleVarDef->action[i] = strdup( l1 ); /** action **/
        rSplitStr( l3, l1, MAX_DVAR_LENGTH, l2, MAX_DVAR_LENGTH, '|' );
        inRuleVarDef->var2CMap[i] = strdup( l1 ); /** var2CMap **/
        if ( strlen( l2 ) > 0 ) {
            inRuleVarDef->varId[i] = atoll( l2 );    /** varId **/
        }
        else {
            inRuleVarDef->varId[i] = i;    /** ruleId **/
        }
        i++;
    }
    fclose( file );
    inRuleVarDef->MaxNumOfDVars = ( long int )  i;
    return 0;
}

int
readFuncMapStructFromFile( char *fmapBaseName, rulefmapdef_t* inRuleFuncMapDef ) {
    int i = 0;
    char l0[MAX_FMAP_LENGTH];
    char l1[MAX_FMAP_LENGTH];
    char l2[MAX_FMAP_LENGTH];
    /*  char l3[MAX_FMAP_LENGTH];*/
    char fmapsFileName[MAX_NAME_LEN];
    FILE *file;
    char buf[MAX_FMAP_LENGTH];
    //char *configDir;

    i = inRuleFuncMapDef->MaxNumOfFMaps;

    if ( fmapBaseName[0] == '/' || fmapBaseName[0] == '\\' ||
            fmapBaseName[1] == ':' ) {
        snprintf( fmapsFileName, MAX_NAME_LEN, "%s", fmapBaseName );
    }
    else {
        std::string cfg_file, fn( fmapBaseName );
        fn += ".fnm";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( fmapsFileName, sizeof( fmapsFileName ), "%s", cfg_file.c_str() );
    }
    file = fopen( fmapsFileName, "r" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "readFmapStructFromFile() could not open fnm file %s\n",
                 fmapsFileName );
        return FMAP_FILE_READ_ERROR;
    }
    buf[MAX_FMAP_LENGTH - 1] = '\0';
    while ( fgets( buf, MAX_FMAP_LENGTH - 1, file ) != NULL ) {
        if ( buf[strlen( buf ) - 1] == '\n' ) {
            buf[strlen( buf ) - 1] = '\0';
        }
        if ( buf[0] == '#' || strlen( buf ) < 4 ) {
            continue;
        }
        rSplitStr( buf, l1, MAX_FMAP_LENGTH, l0, MAX_FMAP_LENGTH, '|' );
        inRuleFuncMapDef->funcName[i] = strdup( l1 ); /** funcName **/
        rSplitStr( l0, l1, MAX_FMAP_LENGTH, l2, MAX_FMAP_LENGTH, '|' );
        inRuleFuncMapDef->func2CMap[i] = strdup( l1 ); /** func2CMap **/
        if ( strlen( l2 ) > 0 ) {
            inRuleFuncMapDef->fmapId[i] = atoll( l2 );    /** fmapId **/
        }
        else {
            inRuleFuncMapDef->fmapId[i] = i;    /** fmapId **/
        }
        i++;
    }
    fclose( file );
    inRuleFuncMapDef->MaxNumOfFMaps = ( long int ) i;
    if ( inRuleFuncMapDef == &coreRuleFuncMapDef ) {
        createFuncMapDefIndex( &coreRuleFuncMapDef, &coreRuleFuncMapDefIndex );
    }
    else if ( inRuleFuncMapDef == &appRuleFuncMapDef ) {
        createFuncMapDefIndex( &appRuleFuncMapDef, &appRuleFuncMapDefIndex );
    }
    return 0;
}

int
readMsrvcStructFromFile( char *msrvcFileName, msrvcStruct_t* inMsrvcStruct ) {
    int i = 0;
    char l0[MAX_RULE_LENGTH];
    char l1[MAX_RULE_LENGTH];
    char l2[MAX_RULE_LENGTH];
    char mymsrvcFileName[MAX_NAME_LEN];
    FILE *file;
    char buf[MAX_RULE_LENGTH];
    char *configDir;


    i = inMsrvcStruct->MaxNumOfMsrvcs;

    if ( msrvcFileName[0] == '/' || msrvcFileName[0] == '\\' ||
            msrvcFileName[1] == ':' ) {
        snprintf( mymsrvcFileName, MAX_NAME_LEN, "%s", msrvcFileName );
    }
    else {
        configDir = getConfigDir();
        snprintf( mymsrvcFileName, MAX_NAME_LEN, "%s/reConfigs/%s.msi", configDir, msrvcFileName );
    }
    file = fopen( mymsrvcFileName, "r" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "readMservcStructFromFile() could not open msrvc file %s\n",
                 mymsrvcFileName );
        return MSRVC_FILE_READ_ERROR;
    }
    buf[MAX_RULE_LENGTH - 1] = '\0';
    while ( fgets( buf, MAX_RULE_LENGTH - 1, file ) != NULL ) {
        if ( buf[strlen( buf ) - 1] == '\n' ) {
            buf[strlen( buf ) - 1] = '\0';
        }
        if ( buf[0] == '#' || strlen( buf ) < 4 ) {
            continue;
        }
        rSplitStr( buf, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->moduleName[i] = strdup( l1 );

        rSplitStr( l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcName[i] = strdup( l1 );
        rSplitStr( l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcSignature[i] = strdup( l1 );

        rSplitStr( l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcVersion[i] = strdup( l1 );
        rSplitStr( l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcHost[i] = strdup( l1 );

        rSplitStr( l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcLocation[i] = strdup( l1 );
        rSplitStr( l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcLanguage[i] = strdup( l1 );

        rSplitStr( l0, l1, MAX_RULE_LENGTH, l2, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcTypeName[i] = strdup( l1 );
        rSplitStr( l2, l1, MAX_RULE_LENGTH, l0, MAX_RULE_LENGTH, '|' );
        inMsrvcStruct->msrvcStatus[i] = atol( l1 );
        if ( strlen( l0 ) > 0 ) {
            inMsrvcStruct->msrvcId[i] = atol( l1 );
        }
        else {
            inMsrvcStruct->msrvcId[i] = ( long int ) i;
        }
        i++;
    }
    fclose( file );
    inMsrvcStruct->MaxNumOfMsrvcs = i;
    return 0;
}

int
findNextRule( const char *action,  int * ruleInx ) {
    int i;
    i = *ruleInx;
    i++;

    if ( i < 0 ) {
        i = 0;
    }
    if ( i < APP_RULE_INDEX_OFF ) {
        for ( ; i < appRuleStrct.MaxNumOfRules && i < MAX_NUM_OF_RULES; i++ ) {
            if ( !strcmp( appRuleStrct.action[i], action ) ) {
                *ruleInx = i;
                return 0;
            }
        }
        i = APP_RULE_INDEX_OFF;
    }
    i  = i - APP_RULE_INDEX_OFF;
    if ( i < CORE_RULE_INDEX_OFF ) {
        for ( ; i < appRuleStrct.MaxNumOfRules && i < MAX_NUM_OF_RULES; i++ ) {
            if ( !strcmp( appRuleStrct.action[i], action ) ) {
                *ruleInx = i;
                return 0;
            }
        }
        i = CORE_RULE_INDEX_OFF;
    }
    i  = i - CORE_RULE_INDEX_OFF;
    for ( ; i < coreRuleStrct.MaxNumOfRules && i < MAX_NUM_OF_RULES; i++ ) {
        if ( !strcmp( coreRuleStrct.action[i], action ) ) {
            *ruleInx = i + APP_RULE_INDEX_OFF;
            return 0;
        }
    }
    return NO_MORE_RULES_ERR;
}



int
getRule( int ri, char *ruleBase, char *ruleHead, char *ruleCondition,
         char *ruleAction, char *ruleRecovery, int rSize ) {

    ruleStruct_t& ruleStruct = ri < CORE_RULE_INDEX_OFF ? appRuleStrct : coreRuleStrct;
    int rule_index = ri < CORE_RULE_INDEX_OFF ? ri : ri - CORE_RULE_INDEX_OFF;
    if ( rule_index >= MAX_NUM_OF_RULES ) {
        rodsLog( LOG_ERROR, "Failed to get rule at %d", ri );
        return SYS_RULE_NOT_FOUND;
    }
    rstrcpy( ruleBase , ruleStruct.ruleBase[rule_index], rSize );
    rstrcpy( ruleHead , ruleStruct.ruleHead[rule_index], rSize );
    rstrcpy( ruleCondition , ruleStruct.ruleCondition[rule_index], rSize );
    rstrcpy( ruleAction , ruleStruct.ruleAction[rule_index], rSize );
    rstrcpy( ruleRecovery , ruleStruct.ruleRecovery[rule_index], rSize );
    return 0;
}

int
insertRulesIntoDBNew( char * baseName, RuleSet *ruleSet,
                      ruleExecInfo_t *rei ) {
    generalRowInsertInp_t generalRowInsertInp;
    char ruleIdStr[MAX_NAME_LEN];
    int rc1, i;
    int  mapPriorityInt = 1;
    char mapPriorityStr[50];
    endTransactionInp_t endTransactionInp;
    char myTime[50];

    memset( &endTransactionInp, 0, sizeof( endTransactionInp_t ) );
    getNowStr( myTime );

    /* Before inserting rules and its base map, we need to first version out the base map */
    generalRowInsertInp.tableName = "versionRuleBase";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = myTime;

    rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
    if ( rc1 < 0 ) {
        endTransactionInp.arg0 = "rollback";
        rsEndTransaction( rei->rsComm, &endTransactionInp );
        return rc1;
    }

    for ( i = 0; i < ruleSet->len; i++ ) {
        RuleDesc *rd = ruleSet->rules[i];
        char *ruleType = NULL;
        Node *ruleNode = rd->node;
        char ruleNameStr[MAX_RULE_LEN];
        char ruleCondStr[MAX_RULE_LEN];
        char ruleActionRecoveryStr[MAX_RULE_LEN];
        Node *avu;
        int s;
        char *p;
        switch ( rd->ruleType ) {
        case RK_FUNC:
            ruleType = "@FUNC";
            p = ruleNameStr;
            s = MAX_RULE_LEN;
            ruleNameToString( &p, &s, 0, ruleNode->subtrees[0] );
            p = ruleCondStr;
            s = MAX_RULE_LEN;
            termToString( &p, &s, 0, MIN_PREC, ruleNode->subtrees[1], 0 );
            p = ruleActionRecoveryStr;
            s = MAX_RULE_LEN;
            termToString( &p, &s, 0, MIN_PREC, ruleNode->subtrees[2], 0 );
            avu = lookupAVUFromMetadata( ruleNode->subtrees[4], "id" );
            if ( avu != NULL ) {
                rstrcpy( ruleIdStr, avu->subtrees[1]->text, MAX_NAME_LEN );
            }
            else {
                rstrcpy( ruleIdStr, "", MAX_NAME_LEN );
            }
            break;
        case RK_REL:
            ruleType = "@REL";
            p = ruleNameStr;
            s = MAX_RULE_LEN;
            ruleNameToString( &p, &s, 0, ruleNode->subtrees[0] );
            p = ruleCondStr;
            s = MAX_RULE_LEN;
            termToString( &p, &s, 0, MIN_PREC, ruleNode->subtrees[1], 0 );
            p = ruleActionRecoveryStr;
            s = MAX_RULE_LEN;
            actionsToString( &p, &s, 0, ruleNode->subtrees[2], ruleNode->subtrees[3] );
            avu = lookupAVUFromMetadata( ruleNode->subtrees[4], "id" );
            if ( avu != NULL ) {
                rstrcpy( ruleIdStr, avu->subtrees[1]->text, MAX_NAME_LEN );
            }
            else {
                rstrcpy( ruleIdStr, "", MAX_NAME_LEN );
            }
            break;
        case RK_DATA:
            ruleType = "@DATA";
            p = ruleNameStr;
            s = MAX_RULE_LEN;
            ruleNameToString( &p, &s, 0, ruleNode->subtrees[0] );
            rstrcpy( ruleCondStr, "", MAX_RULE_LEN );
            rstrcpy( ruleActionRecoveryStr, "", MAX_RULE_LEN );
            rstrcpy( ruleIdStr, "", MAX_NAME_LEN );
            break;
        case RK_CONSTRUCTOR:
            ruleType = "@CONSTR";
            rstrcpy( ruleNameStr, ruleNode->subtrees[0]->text, MAX_RULE_LEN );
            rstrcpy( ruleCondStr, "", MAX_RULE_LEN );
            p = ruleActionRecoveryStr;
            s = MAX_RULE_LEN;
            typeToStringParser( &p, &s, 0, 0, ruleNode->subtrees[1] );
            rstrcpy( ruleIdStr, "", MAX_NAME_LEN );
            break;
        case RK_EXTERN:
            ruleType = "@EXTERN";
            rstrcpy( ruleNameStr, ruleNode->subtrees[0]->text, MAX_RULE_LEN );
            rstrcpy( ruleCondStr, "", MAX_RULE_LEN );
            p = ruleActionRecoveryStr;
            s = MAX_RULE_LEN;
            typeToStringParser( &p, &s, 0, 0, ruleNode->subtrees[1] );
            rstrcpy( ruleIdStr, "", MAX_NAME_LEN );
            break;
        }
        generalRowInsertInp.tableName = "ruleTable";
        generalRowInsertInp.arg1 = baseName;
        sprintf( mapPriorityStr, "%i", mapPriorityInt );
        mapPriorityInt++;
        generalRowInsertInp.arg2 = mapPriorityStr;
        generalRowInsertInp.arg3 = ruleNode->subtrees[0]->text;
        generalRowInsertInp.arg4 = ruleNameStr;
        generalRowInsertInp.arg5 = ruleCondStr;
        generalRowInsertInp.arg6 = ruleActionRecoveryStr;
        generalRowInsertInp.arg7 = ruleType;
        generalRowInsertInp.arg8 = ruleIdStr;
        generalRowInsertInp.arg9 = myTime;

        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
        if ( rc1 < 0 ) {
            endTransactionInp.arg0 = "rollback";
            rsEndTransaction( rei->rsComm, &endTransactionInp );
            return rc1;
        }
    }

    endTransactionInp.arg0 = "commit";
    rc1 = rsEndTransaction( rei->rsComm, &endTransactionInp );
    return rc1;
}

int
insertRulesIntoDB( char * baseName, ruleStruct_t *coreRuleStruct,
                   ruleExecInfo_t *rei ) {
    generalRowInsertInp_t generalRowInsertInp;
    char ruleIdStr[MAX_NAME_LEN];
    int rc1, i;
    int  mapPriorityInt = 1;
    char mapPriorityStr[50];
    endTransactionInp_t endTransactionInp;
    char myTime[50];

    memset( &endTransactionInp, 0, sizeof( endTransactionInp_t ) );
    getNowStr( myTime );

    /* Before inserting rules and its base map, we need to first version out the base map */
    generalRowInsertInp.tableName = "versionRuleBase";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = myTime;

    rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
    if ( rc1 < 0 ) {
        endTransactionInp.arg0 = "rollback";
        rsEndTransaction( rei->rsComm, &endTransactionInp );
        return rc1;
    }

    for ( i = 0; i < coreRuleStruct->MaxNumOfRules; i++ ) {
        generalRowInsertInp.tableName = "ruleTable";
        generalRowInsertInp.arg1 = baseName;
        sprintf( mapPriorityStr, "%i", mapPriorityInt );
        mapPriorityInt++;
        generalRowInsertInp.arg2 = mapPriorityStr;
        generalRowInsertInp.arg3 = coreRuleStruct->action[i];
        generalRowInsertInp.arg4 = coreRuleStruct->ruleHead[i];
        generalRowInsertInp.arg5 = coreRuleStruct->ruleCondition[i];
        generalRowInsertInp.arg6 = coreRuleStruct->ruleAction[i];
        generalRowInsertInp.arg7 = coreRuleStruct->ruleRecovery[i];
        generalRowInsertInp.arg8 = ruleIdStr;
        generalRowInsertInp.arg9 = myTime;

        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
        if ( rc1 < 0 ) {
            endTransactionInp.arg0 = "rollback";
            rsEndTransaction( rei->rsComm, &endTransactionInp );
            return rc1;
        }
    }

    endTransactionInp.arg0 = "commit";
    rc1 = rsEndTransaction( rei->rsComm, &endTransactionInp );
    return rc1;
}

int
insertDVMapsIntoDB( char * baseName, dvmStruct_t *coreDVMStruct,
                    ruleExecInfo_t *rei ) {
    generalRowInsertInp_t generalRowInsertInp;
    int rc1, i;
    endTransactionInp_t endTransactionInp;
    char myTime[50];

    memset( &endTransactionInp, 0, sizeof( endTransactionInp_t ) );
    getNowStr( myTime );

    /* Before inserting rules and its base map, we need to first version out the base map */
    generalRowInsertInp.tableName = "versionDVMBase";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = myTime;

    rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
    if ( rc1 < 0 ) {
        endTransactionInp.arg0 = "rollback";
        rsEndTransaction( rei->rsComm, &endTransactionInp );
        return rc1;
    }

    for ( i = 0; i < coreDVMStruct->MaxNumOfDVars; i++ ) {
        generalRowInsertInp.tableName = "dvmTable";
        generalRowInsertInp.arg1 = baseName;
        generalRowInsertInp.arg2 = coreDVMStruct->varName[i];
        generalRowInsertInp.arg3 = coreDVMStruct->action[i];
        generalRowInsertInp.arg4 = coreDVMStruct->var2CMap[i];
        generalRowInsertInp.arg5 = myTime;

        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
        if ( rc1 < 0 ) {
            endTransactionInp.arg0 = "rollback";
            rsEndTransaction( rei->rsComm, &endTransactionInp );
            return rc1;
        }
    }

    endTransactionInp.arg0 = "commit";
    rc1 = rsEndTransaction( rei->rsComm, &endTransactionInp );
    return rc1;
}

int
insertFNMapsIntoDB( char * baseName, fnmapStruct_t *coreFNMStruct,
                    ruleExecInfo_t *rei ) {
    generalRowInsertInp_t generalRowInsertInp;
    int rc1, i;
    endTransactionInp_t endTransactionInp;
    char myTime[50];

    memset( &endTransactionInp, 0, sizeof( endTransactionInp_t ) );
    getNowStr( myTime );

    /* Before inserting rules and its base map, we need to first version out the base map */
    generalRowInsertInp.tableName = "versionFNMBase";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = myTime;

    rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
    if ( rc1 < 0 ) {
        endTransactionInp.arg0 = "rollback";
        rsEndTransaction( rei->rsComm, &endTransactionInp );
        return rc1;
    }

    for ( i = 0; i < coreFNMStruct->MaxNumOfFMaps; i++ ) {
        generalRowInsertInp.tableName = "fnmTable";
        generalRowInsertInp.arg1 = baseName;
        generalRowInsertInp.arg2 = coreFNMStruct->funcName[i];
        generalRowInsertInp.arg3 = coreFNMStruct->func2CMap[i];
        generalRowInsertInp.arg4 = myTime;

        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
        if ( rc1 < 0 ) {
            endTransactionInp.arg0 = "rollback";
            rsEndTransaction( rei->rsComm, &endTransactionInp );
            return rc1;
        }
    }

    endTransactionInp.arg0 = "commit";
    rc1 = rsEndTransaction( rei->rsComm, &endTransactionInp );
    return rc1;
}



int
insertMSrvcsIntoDB( msrvcStruct_t *coreMsrvcStruct,
                    ruleExecInfo_t *rei ) {

    generalRowInsertInp_t generalRowInsertInp;
    int rc1, i;
    endTransactionInp_t endTransactionInp;
    char myTime[100];
    char myStatus[100];
    memset( &endTransactionInp, 0, sizeof( endTransactionInp_t ) );
    getNowStr( myTime );

    for ( i = 0; i < coreMsrvcStruct->MaxNumOfMsrvcs; i++ ) {
        generalRowInsertInp.tableName = "msrvcTable";
        generalRowInsertInp.arg1 = coreMsrvcStruct->moduleName[i];
        generalRowInsertInp.arg2 = coreMsrvcStruct->msrvcName[i];
        generalRowInsertInp.arg3 = coreMsrvcStruct->msrvcSignature[i];
        generalRowInsertInp.arg4 = coreMsrvcStruct->msrvcVersion[i];
        generalRowInsertInp.arg5 = coreMsrvcStruct->msrvcHost[i];
        generalRowInsertInp.arg6 = coreMsrvcStruct->msrvcLocation[i];
        generalRowInsertInp.arg7 = coreMsrvcStruct->msrvcLanguage[i];
        generalRowInsertInp.arg8 = coreMsrvcStruct->msrvcTypeName[i];
        snprintf( myStatus, 100, "%ld", coreMsrvcStruct->msrvcStatus[i] );
        generalRowInsertInp.arg9 = myStatus;
        generalRowInsertInp.arg10 = myTime;


        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
        if ( rc1 < 0 ) {
            endTransactionInp.arg0 = "rollback";
            rsEndTransaction( rei->rsComm, &endTransactionInp );
            return rc1;
        }
    }

    endTransactionInp.arg0 = "commit";
    rc1 = rsEndTransaction( rei->rsComm, &endTransactionInp );
    return rc1;

}


int
writeRulesIntoFile( char * inFileName, ruleStruct_t *myRuleStruct,
                    ruleExecInfo_t* ) {

    int i;
    FILE *file;
    char fileName[MAX_NAME_LEN];
    //char *configDir;

    if ( inFileName[0] == '/' || inFileName[0] == '\\' ||
            inFileName[1] == ':' ) {
        snprintf( fileName, MAX_NAME_LEN, "%s", inFileName );
    }
    else {
        std::string cfg_file, fn( inFileName );
        fn += ".irb";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( fileName, sizeof( fileName ), "%s", cfg_file.c_str() );

    }


    file = fopen( fileName, "w" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "writeRulesIntoFile() could not open rules file %s for writing\n",
                 fileName );
        return FILE_OPEN_ERR;
    }
    for ( i = 0; i < myRuleStruct->MaxNumOfRules; i++ ) {
        fprintf( file, "%s|%s|%s|%s|%ld\n", myRuleStruct->ruleHead[i],
                 myRuleStruct->ruleCondition[i],
                 myRuleStruct->ruleAction[i],
                 myRuleStruct->ruleRecovery[i],
                 myRuleStruct->ruleId[i] );

    }
    fclose( file );
    return 0;
}

int
writeDVMapsIntoFile( char * inFileName, dvmStruct_t *myDVMapStruct,
                     ruleExecInfo_t* ) {
    int i;
    FILE *file;
    char fileName[MAX_NAME_LEN];

    if ( inFileName[0] == '/' || inFileName[0] == '\\' ||
            inFileName[1] == ':' ) {
        snprintf( fileName, MAX_NAME_LEN, "%s", inFileName );
    }
    else {
        std::string cfg_file, fn( inFileName );
        fn += ".dvm";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( fileName, sizeof( fileName ), "%s", cfg_file.c_str() );
    }

    file = fopen( fileName, "w" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "writeDVMapsIntoFile() could not open rules file %s for writing\n",
                 fileName );
        return FILE_OPEN_ERR;
    }
    for ( i = 0; i < myDVMapStruct->MaxNumOfDVars; i++ ) {
        fprintf( file, "%s|%s|%s|%ld\n", myDVMapStruct->varName[i],
                 myDVMapStruct->action[i],
                 myDVMapStruct->var2CMap[i],
                 myDVMapStruct->varId[i] );
    }
    fclose( file );
    return 0;
}


int
writeFNMapsIntoFile( char * inFileName, fnmapStruct_t *myFNMapStruct,
                     ruleExecInfo_t* ) {
    int i;
    FILE *file;
    char fileName[MAX_NAME_LEN];

    if ( inFileName[0] == '/' || inFileName[0] == '\\' ||
            inFileName[1] == ':' ) {
        snprintf( fileName, MAX_NAME_LEN, "%s", inFileName );
    } else {
        std::string cfg_file, fn( inFileName );
        fn += ".fnm";
        irods::error ret = irods::get_full_path_for_config_file( fn, cfg_file );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        snprintf( fileName, sizeof( fileName ), "%s", cfg_file.c_str() );
    }

    file = fopen( fileName, "w" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "writeFNMapsIntoFile() could not open rules file %s for writing\n",
                 fileName );
        return FILE_OPEN_ERR;
    }
    for ( i = 0; i < myFNMapStruct->MaxNumOfFMaps; i++ ) {
        fprintf( file, "%s|%s|%ld\n", myFNMapStruct->funcName[i],
                 myFNMapStruct->func2CMap[i],
                 myFNMapStruct->fmapId[i] );
    }
    fclose( file );
    return 0;
}



int
writeMSrvcsIntoFile( char * inFileName, msrvcStruct_t *myMsrvcStruct,
                     ruleExecInfo_t* ) {
    int i;
    FILE *file;
    char fileName[MAX_NAME_LEN];
    char *configDir;

    if ( inFileName[0] == '/' || inFileName[0] == '\\' ||
            inFileName[1] == ':' ) {
        snprintf( fileName, MAX_NAME_LEN, "%s", inFileName );
    }
    else {
        configDir = getConfigDir();
        snprintf( fileName, MAX_NAME_LEN, "%s/reConfigs/%s.msi", configDir, inFileName );
    }

    file = fopen( fileName, "w" );
    if ( file == NULL ) {
        rodsLog( LOG_NOTICE,
                 "writeMsrvcsIntoFile() could not open microservics file %s for writing\n",
                 fileName );
        return FILE_OPEN_ERR;
    }
    for ( i = 0; i < myMsrvcStruct->MaxNumOfMsrvcs; i++ ) {
        fprintf( file, "%s|%s|%s|%s|%s|%s|%s|%s|%ld|%ld\n",
                 myMsrvcStruct->moduleName[i],
                 myMsrvcStruct->msrvcName[i],
                 myMsrvcStruct->msrvcSignature[i],
                 myMsrvcStruct->msrvcVersion[i],
                 myMsrvcStruct->msrvcHost[i],
                 myMsrvcStruct->msrvcLocation[i],
                 myMsrvcStruct->msrvcLanguage[i],
                 myMsrvcStruct->msrvcTypeName[i],
                 myMsrvcStruct->msrvcStatus[i],
                 myMsrvcStruct->msrvcId[i] );
    }
    fclose( file );
    return 0;
}

int
finalizeRuleEngine() {
    if ( GlobalREDebugFlag > 5 ) {
        _writeXMsg( GlobalREDebugFlag, "idbug", "PROCESS END" );
    }
    return 0;
}
