/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reHelpers1.hpp"

#include "reFuncDefs.hpp"
#include "index.hpp"
#include "parser.hpp"
#include "filesystem.hpp"
#include "configuration.hpp"

#include "rcMisc.h"
#include "irods_log.hpp"


struct Breakpoint {
    char *actionName;
    char *base;
    int pending;
    int row;
    int line;
    rodsLong_t start;
    rodsLong_t finish; /* exclusive */
} breakPoints[100];

int breakPointsInx = 0;

int reDebugPCType( RuleEngineEvent label );
void printRuleEngineEventLabel( char buf[], size_t n, RuleEngineEvent label, RuleEngineEventParam *param );


void disableReDebugger( int grdf[2] ) {
    grdf[0] = GlobalREAuditFlag;
    grdf[1] = GlobalREDebugFlag;
    GlobalREAuditFlag = 0;
    GlobalREDebugFlag = 0;

}

void enableReDebugger( int grdf[2] ) {
    GlobalREAuditFlag = grdf[0];
    GlobalREDebugFlag = grdf[1];

}
char myHostName[MAX_NAME_LEN];
char waitHdr[HEADER_TYPE_LEN];
char waitMsg[MAX_NAME_LEN];
int myPID;

int initializeReDebug( rsComm_t *svrComm ) {
    char condRead[NAME_LEN];
    int i, s, m, status;
    char *readhdr = NULL;
    char *readmsg = NULL;
    char *user = NULL;
    char *addr = NULL;

    if ( svrComm == NULL ) {
        return 0;
    }
    if ( GlobalREDebugFlag != 4 ) {
        return 0;
    }

    s = 0;
    m = 0;
    myPID = ( int ) getpid();
    myHostName[0] = '\0';
    gethostname( myHostName, MAX_NAME_LEN );
    sprintf( condRead, "(*XUSER  == \"%s@%s\") && (*XHDR == \"STARTDEBUG\")",
             svrComm->clientUser.userName, svrComm->clientUser.rodsZone );
    status = _readXMsg( GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr );
    if ( status >= 0 ) {
        if ( ( readmsg != NULL )  && strlen( readmsg ) > 0 ) {
            GlobalREDebugFlag = atoi( readmsg );
        }
        if ( readhdr != NULL ) {
            free( readhdr );
        }
        if ( readmsg != NULL ) {
            free( readmsg );
        }
        if ( user != NULL ) {
            free( user );
        }
        if ( addr != NULL ) {
            free( addr );
        }
        /* initialize reDebug stack space*/
        for ( i = 0; i < REDEBUG_STACK_SIZE_FULL; i++ ) {
            reDebugStackFull[i] = NULL;
        }
        for ( i = 0; i < REDEBUG_STACK_SIZE_CURR; i++ ) {
            reDebugStackCurr[i].label = -1;
            reDebugStackCurr[i].step = NULL;
        }
        memset( breakPoints, 0, sizeof( struct Breakpoint ) * 100 );

        reDebugStackFullPtr = 0;
        reDebugStackCurrPtr = 0;
        snprintf( waitHdr, HEADER_TYPE_LEN - 1,   "idbug:" );

        rodsLog( LOG_NOTICE, "reDebugInitialization: Got Debug StreamId:%i\n", GlobalREDebugFlag );
        snprintf( waitMsg, MAX_NAME_LEN, "PROCESS BEGIN at %s:%i. Client connected from %s at port %i\n",
                  myHostName, myPID, svrComm->clientAddr, ntohs( svrComm->localAddr.sin_port ) );
        _writeXMsg( GlobalREDebugFlag, "idbug", waitMsg );
        snprintf( waitMsg, MAX_NAME_LEN, "%s:%i is waiting\n", myHostName, myPID );
    }
    return 0;
}


int processXMsg( int streamId, char *readmsg,
                 RuleEngineEventParam *param, Node *node,
                 Env *env, ruleExecInfo_t *rei ) {

    char myhdr[HEADER_TYPE_LEN];
    char mymsg[MAX_NAME_LEN];
    char *outStr = NULL;
    int i, n;
    int iLevel, wCnt;
    int  ruleInx = 0;
    Region *r;
    Res *res;
    rError_t errmsg;
    errmsg.len = 0;
    errmsg.errMsg = NULL;
    r = make_region( 0, NULL );
    ParserContext *context = newParserContext( &errmsg, r );
    Pointer *e = newPointer2( readmsg );
    int rulegen = 1;
    int found;
    int grdf[2];
    int cmd = 0;
    int smallW;
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s", param->actionName );
    PARSER_BEGIN( DbgCmd )
    TRY( DbgCmd )
    TTEXT2( "n", "next" );
    cmd = REDEBUG_STEP_OVER;
    OR( DbgCmd )
    TTEXT2( "s", "step" );
    cmd = REDEBUG_NEXT;
    OR( DbgCmd )
    TTEXT2( "f", "finish" );
    cmd = REDEBUG_STEP_OUT;
    OR( DbgCmd )
    TTEXT2( "b", "break" );
    TRY( Param )
    TTYPE( TK_TEXT );
    int breakPointsInx2;
    for ( breakPointsInx2 = 0; breakPointsInx2 < 100; breakPointsInx2++ ) {
        if ( breakPoints[breakPointsInx2].actionName == NULL ) {
            break;
        }
    }
    if ( breakPointsInx2 == 100 ) {
        _writeXMsg( streamId, myhdr, "Maximum breakpoint count reached. Breakpoint not set.\n" );
        cmd = REDEBUG_WAIT;
    }
    else {
        breakPoints[breakPointsInx2].actionName = strdup( token->text );
        char * base_ptr = NULL;
        TRY( loc )
        TTYPE( TK_TEXT );
        base_ptr = ( char * ) malloc( sizeof( token->text ) + 2 );
        base_ptr[0] = 'f';
        strcpy( base_ptr + 1, token->text );
        TTEXT( ":" );
        TTYPE( TK_INT );
        breakPoints[breakPointsInx2].base = strdup( base_ptr );
        breakPoints[breakPointsInx2].line = atoi( token->text );
        rodsLong_t range[2];
        char rulesFileName[MAX_NAME_LEN];
        getRuleBasePath( base_ptr, rulesFileName );

        FILE *file;
        /* char errbuf[ERR_MSG_LEN]; */
        file = fopen( rulesFileName, "r" );
        if ( file == NULL ) {
            free( context );
            deletePointer( e );
            free( base_ptr );
            return RULES_FILE_READ_ERROR;
        }
        Pointer *p = newPointer( file, base_ptr );


        if ( getLineRange( p, breakPoints[breakPointsInx2].line, range ) == 0 ) {
            breakPoints[breakPointsInx2].start = range[0];
            breakPoints[breakPointsInx2].finish = range[1];
        }
        else {
            breakPoints[breakPointsInx2].actionName = NULL;
        }
        deletePointer( p );
        OR( loc )
        TTYPE( TK_INT );
        if ( node != NULL ) {
            breakPoints[breakPointsInx2].base = strdup( node->base );
            breakPoints[breakPointsInx2].line = atoi( token->text );
            rodsLong_t range[2];
            Pointer *p = newPointer2( breakPoints[breakPointsInx2].base );
            if ( getLineRange( p, breakPoints[breakPointsInx2].line, range ) == 0 ) {
                breakPoints[breakPointsInx2].start = range[0];
                breakPoints[breakPointsInx2].finish = range[1];
            }
            else {
                breakPoints[breakPointsInx2].actionName = NULL;
            }
            deletePointer( p );
        }
        else {
            breakPoints[breakPointsInx2].actionName = NULL;
        }
        OR( loc )
        /* breakPoints[breakPointsInx].base = NULL; */
        END_TRY( loc )

        free( base_ptr );
        if ( breakPoints[breakPointsInx2].actionName != NULL )
            snprintf( mymsg, MAX_NAME_LEN, "Breakpoint %i  set at %s\n", breakPointsInx2,
                      breakPoints[breakPointsInx2].actionName );
        else {
            snprintf( mymsg, MAX_NAME_LEN, "Cannot set breakpoint, source not available\n" );
        }
        _writeXMsg( streamId, myhdr, mymsg );
        if ( breakPointsInx <= breakPointsInx2 ) {
            breakPointsInx = breakPointsInx2 + 1;
        }
        cmd = REDEBUG_WAIT;
    }
    OR( Param )
    NEXT_TOKEN_BASIC;
    _writeXMsg( streamId, myhdr, "Unknown parameter type.\n" );
    cmd = REDEBUG_WAIT;
    OR( Param )
    _writeXMsg( streamId, myhdr, "Debugger command \'break\' requires at least one argument.\n" );
    cmd = REDEBUG_WAIT;
    END_TRY( Param )

    OR( DbgCmd )
    TRY( Where )
    TTEXT2( "w", "where" );
    smallW = 1;
    OR( Where )
    TTEXT2( "W", "Where" );
    smallW = 0;
    END_TRY( Where )
    wCnt = 20;
    OPTIONAL_BEGIN( Param )
    TTYPE( TK_INT );
    wCnt = atoi( token->text );
    OPTIONAL_END( Param )
    iLevel = 0;

    i = reDebugStackCurrPtr - 1;
    while ( i >= 0 && wCnt > 0 ) {
        if ( !smallW || ( reDebugPCType( ( RuleEngineEvent ) reDebugStackCurr[i].label ) & 1 ) != 0 ) {
            snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Level %3i", iLevel );
            char msg[HEADER_TYPE_LEN - 1];
            RuleEngineEventParam param;
            param.ruleIndex = 0;
            param.actionName = reDebugStackCurr[i].step;
            printRuleEngineEventLabel( msg, HEADER_TYPE_LEN - 1, ( RuleEngineEvent ) reDebugStackCurr[i].label, &param );
            _writeXMsg( streamId,  myhdr, msg );
            if ( reDebugStackCurr[i].label != EXEC_ACTION_BEGIN ) {
                iLevel++;
            }
            wCnt--;
        }
        i--;
    }
    OR( DbgCmd )
    TTEXT2( "l", "list" );
    TRY( Param )
    TTEXT2( "r", "rule" );
    TRY( ParamParam )
    TTYPE( TK_TEXT );

    mymsg[0] = '\n';
    mymsg[1] = '\0';
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s", token->text );
    RuleIndexListNode *node;
    found = 0;
    while ( findNextRule2( token->text, ruleInx, &node ) != NO_MORE_RULES_ERR ) {
        found = 1;
        if ( node->secondaryIndex ) {
            n = node->condIndex->valIndex->len;
            int i;
            for ( i = 0; i < n; i++ ) {
                Bucket *b = node->condIndex->valIndex->buckets[i];
                while ( b != NULL ) {
                    RuleDesc *rd = getRuleDesc( *( int * )b->value );
                    char buf[MAX_RULE_LEN];
                    ruleToString( buf, MAX_RULE_LEN, rd );
                    snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "%i: %s\n%s\n", node->ruleIndex, rd->node->base[0] == 's' ? "<source>" : rd->node->base + 1, buf );
                    b = b->next;
                }
            }
        }
        else {
            RuleDesc *rd = getRuleDesc( node->ruleIndex );
            char buf[MAX_RULE_LEN];
            ruleToString( buf, MAX_RULE_LEN, rd );
            snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "\n  %i: %s\n%s\n", node->ruleIndex, rd->node->base[0] == 's' ? "<source>" : rd->node->base + 1, buf );
        }
        ruleInx ++;
    }
    if ( !found ) {
        snprintf( mymsg, MAX_NAME_LEN, "Rule %s not found\n", token->text );
    }
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    OR( ParamParam )
    _writeXMsg( streamId, myhdr, "Debugger command \'list rule\' requires one argument.\n" );
    cmd = REDEBUG_WAIT;
    END_TRY( ParamParam )
    OR( Param )
    TTEXT2( "b", "breakpoints" );
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s", token->text );
    mymsg[0] = '\n';
    mymsg[1] = '\0';
    for ( i = 0; i < breakPointsInx; i++ ) {
        if ( breakPoints[i].actionName != NULL ) {
            if ( breakPoints[i].base != NULL ) {
                snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "Breaking at BreakPoint %i:%s %s:%d\n", i , breakPoints[i].actionName, breakPoints[i].base[0] == 's' ? "<source>" : breakPoints[i].base + 1, breakPoints[i].line );
            }
            else {
                snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "Breaking at BreakPoint %i:%s\n", i , breakPoints[i].actionName );
            }
        }
    }
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    OR( Param )
    TTEXT( "*" );
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s", token->text );
    Env *cenv = env;
    mymsg[0] = '\n';
    mymsg[1] = '\0';
    found = 0;
    while ( cenv != NULL ) {
        n = cenv->current->size;
        for ( i = 0; i < n; i++ ) {
            Bucket *b = cenv->current->buckets[i];
            while ( b != NULL ) {
                if ( b->key[0] == '*' ) { /* skip none * variables */
                    found = 1;
                    char typeString[128];
                    typeToString( ( ( Res * )b->value )->exprType, NULL, typeString, 128 );
                    snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "%s of type %s\n", b->key, typeString );
                }
                b = b->next;
            }
        }
        cenv = cenv->previous;
    }
    if ( !found ) {
        snprintf( mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ), "<empty>\n" );
    }
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    OR( Param )
    syncTokenQueue( e, context );
    skipWhitespace( e );
    ABORT( lookAhead( e, 0 ) != '$' );
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s", token->text );
    mymsg[0] = '\n';
    mymsg[1] = '\0';
    Hashtable *vars = newHashTable( 100 );
    for ( i = 0; i < coreRuleVarDef .MaxNumOfDVars; i++ ) {
        if ( lookupFromHashTable( vars, coreRuleVarDef.varName[i] ) == NULL ) {
            snprintf( &mymsg[strlen( mymsg )], MAX_NAME_LEN - strlen( mymsg ), "$%s\n", coreRuleVarDef.varName[i] );
            insertIntoHashTable( vars, coreRuleVarDef.varName[i], coreRuleVarDef.varName[i] );
        }
    }
    deleteHashTable( vars, NULL );
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    OR( Param )
    NEXT_TOKEN_BASIC;
    _writeXMsg( streamId, myhdr, "Unknown parameter type.\n" );
    cmd = REDEBUG_WAIT;
    OR( Param )
    _writeXMsg( streamId, myhdr, "Debugger command \'list\' requires at least one argument.\n" );
    cmd = REDEBUG_WAIT;
    END_TRY( Param )
    OR( DbgCmd )
    TTEXT2( "c", "continue" );
    cmd = REDEBUG_STEP_CONTINUE;
    OR( DbgCmd )
    TTEXT2( "C", "Continue" );
    cmd = REDEBUG_CONTINUE_VERBOSE;
    OR( DbgCmd )
    TTEXT2( "del", "delete" );
    TRY( Param )
    TTYPE( TK_INT );
    n = atoi( token->text );
    if ( breakPoints[n].actionName != NULL ) {
        free( breakPoints[n].actionName );
        if ( breakPoints[n].base != NULL ) {
            free( breakPoints[n].base );
        }
        breakPoints[n].actionName = NULL;
        breakPoints[n].base = NULL;
        snprintf( mymsg, MAX_NAME_LEN, "Breakpoint %i  deleted\n", n );
    }
    else {
        snprintf( mymsg, MAX_NAME_LEN, "Breakpoint %i  has not been defined\n", n );
    }
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    OR( Param )
    _writeXMsg( streamId, myhdr, "Debugger command \'delete\' requires one argument.\n" );
    cmd = REDEBUG_WAIT;
    END_TRY( Param )
    OR( DbgCmd )
    TTEXT2( "p", "print" );
    Node *n = parseTermRuleGen( e, 1, context );
    if ( getNodeType( n ) == N_ERROR ) {
        errMsgToString( context->errmsg, mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ) );
    }
    else {
        snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug: Printing " );
        char * ptr = myhdr + strlen( myhdr );
        i = HEADER_TYPE_LEN - 1 - strlen( myhdr );
        termToString( &ptr, &i, 0, MIN_PREC, n, 0 );
        snprintf( ptr, i, "\n" );
        if ( env != NULL ) {
            disableReDebugger( grdf );
            res = computeNode( n, NULL, regionRegionCpEnv( env, r, ( RegionRegionCopyFuncType * ) regionRegionCpNode ), rei, 0, &errmsg, r );
            enableReDebugger( grdf );
            outStr = convertResToString( res );
            snprintf( mymsg, MAX_NAME_LEN, "%s\n", outStr );
            free( outStr );
            if ( getNodeType( res ) == N_ERROR ) {
                errMsgToString( &errmsg, mymsg + strlen( mymsg ), MAX_NAME_LEN - strlen( mymsg ) );
            }
        }
        else {
            snprintf( mymsg, MAX_NAME_LEN, "Runtime environment: <empty>\n" );
        }
    }
    _writeXMsg( streamId, myhdr, mymsg );

    cmd = REDEBUG_WAIT;
    OR( DbgCmd )
    TTEXT2( "d", "discontinue" );
    cmd = REDEBUG_WAIT;
    OR( DbgCmd )
    snprintf( mymsg, MAX_NAME_LEN, "Unknown Action: %s", readmsg );
    _writeXMsg( streamId, myhdr, mymsg );
    cmd = REDEBUG_WAIT;
    END_TRY( DbgCmd )
    PARSER_END( DbgCmd )
    freeRErrorContent( &errmsg );
    region_free( r );
    deletePointer( e );
    free( context );
    return cmd;
}

int
processBreakPoint( int streamId, RuleEngineEventParam *param,
                   Node *node, int curStat ) {

    char myhdr[HEADER_TYPE_LEN];
    snprintf( myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s", param->actionName );

    if ( breakPointsInx > 0 && node != NULL ) {
        for ( int i = 0; i < breakPointsInx; i++ ) {
            if ( breakPoints[i].base != NULL && breakPoints[i].actionName != NULL &&
                    node->expr >= breakPoints[i].start && node->expr < breakPoints[i].finish &&
                    strcmp( node->base, breakPoints[i].base ) == 0 &&
                    strncmp( param->actionName, breakPoints[i].actionName, strlen( breakPoints[i].actionName ) ) == 0 ) {
                char buf[MAX_NAME_LEN];
                snprintf( buf, MAX_NAME_LEN, "Breaking at BreakPoint %i:%s\n", i , breakPoints[i].actionName );
                char mymsg[MAX_NAME_LEN];
                generateErrMsg( buf, node->expr, node->base, mymsg );
                _writeXMsg( streamId, myhdr, mymsg );
                snprintf( mymsg, MAX_NAME_LEN, "%s\n", param->actionName );
                _writeXMsg( streamId, myhdr, mymsg );
                return REDEBUG_WAIT;
            }
        }
    }
    return curStat;
}


/* char *stackStr;
char *stackStr2;
char *s; */


/*stackStr = (char *) malloc(strlen(hdr) + strlen(step) + 5);
sprintf(stackStr,"%s: %s\n", hdr, step);


i = reDebugStackFullPtr;
if (reDebugStackFull[i] != NULL) {
  free(reDebugStackFull[i]);
}
reDebugStackFull[i] = stackStr;
reDebugStackFullPtr = (i + 1 ) % REDEBUG_STACK_SIZE_FULL;
i = reDebugStackFullPtr;
if (reDebugStackFull[i] != NULL) {
  free(reDebugStackFull[i]);
  reDebugStackFull[i] = NULL;
}*/
int
popReStack( RuleEngineEvent label, char* step ) {

    int i;
    i = reDebugStackCurrPtr - 1;
    while ( i >= 0 && ( reDebugStackCurr[i].label != label || strcmp( reDebugStackCurr[i].step, step ) != 0 ) ) {
        free( reDebugStackCurr[i].step );
        reDebugStackCurr[i].step = NULL;
        reDebugStackCurr[i].label = -1;
        i = i - 1;
    }
    reDebugStackCurrPtr = i;
    return 0;
}

int
pushReStack( RuleEngineEvent label, char* step ) {

    int i;
    i = reDebugStackCurrPtr;
    if ( i < REDEBUG_STACK_SIZE_CURR ) {
        reDebugStackCurr[i].label = label;
        reDebugStackCurr[i].step = strdup( step );
        reDebugStackCurrPtr = i + 1;
    }
    return 0;
}


int sendWaitXMsg( int streamId ) {
    _writeXMsg( streamId, waitHdr, waitMsg );
    return 0;
}
int cleanUpDebug() {
    int i;
    for ( i = 0 ; i < REDEBUG_STACK_SIZE_CURR; i++ ) {
        if ( reDebugStackCurr[i].step != NULL ) {
            free( reDebugStackCurr[i].step );
            reDebugStackCurr[i].step = NULL;
            reDebugStackCurr[i].label = -1;
        }
    }
    for ( i = 0 ; i < REDEBUG_STACK_SIZE_FULL; i++ ) {
        if ( reDebugStackFull[i] != NULL ) {
            free( reDebugStackFull[i] );
            reDebugStackFull[i] = NULL;
        }
    }
    reDebugStackCurrPtr = 0;
    reDebugStackFullPtr = 0;
    GlobalREDebugFlag = -1;
    return 0;
}

void printRuleEngineEventLabel( char buf[], size_t n, RuleEngineEvent label, RuleEngineEventParam *param ) {
    switch ( label ) {
    case EXEC_RULE_BEGIN:
        snprintf( buf, n, "ExecRule:%s", param->actionName );
        break;
    case EXEC_RULE_END:
        snprintf( buf, n, "ExecRule Done:%s", param->actionName );
        break;
    case EXEC_ACTION_BEGIN:
        snprintf( buf, n, "ExecAction:%s", param->actionName );
        break;
    case EXEC_ACTION_END:
        snprintf( buf, n, "ExecAction Done:%s", param->actionName );
        break;
    case EXEC_MICRO_SERVICE_BEGIN:
        snprintf( buf, n, "ExecMicroSvrc:%s", param->actionName );
        break;
    case EXEC_MICRO_SERVICE_END:
        snprintf( buf, n, "ExecMicroSvrc Done:%s", param->actionName );
        break;
    case GOT_RULE:
        snprintf( buf, n, "GotRule:%s:%d", param->actionName, param->ruleIndex );
        break;
    case APPLY_RULE_BEGIN:
        snprintf( buf, n, "ApplyRule:%s", param->actionName );
        break;
    case APPLY_RULE_END:
        snprintf( buf, n, "ApplyRule Done:%s", param->actionName );
        break;
    case APPLY_ALL_RULES_BEGIN:
        snprintf( buf, n, "ApplyAllRules:%s", param->actionName );
        break;
    case APPLY_ALL_RULES_END:
        snprintf( buf, n, "ApplyAllRules Done:%s", param->actionName );
        break;
    case EXEC_MY_RULE_BEGIN:
        snprintf( buf, n, "ExecMyRule:%s", param->actionName );
        break;
    case EXEC_MY_RULE_END:
        snprintf( buf, n, "ExecMyRule Done:%s", param->actionName );
        break;
    }
}
int reDebugPCType( RuleEngineEvent label ) {
    switch ( label ) {
    case EXEC_RULE_BEGIN:
    case EXEC_ACTION_BEGIN:
    case EXEC_MICRO_SERVICE_BEGIN:
    case APPLY_RULE_BEGIN:
    case APPLY_ALL_RULES_BEGIN:
    case EXEC_MY_RULE_BEGIN:
        return 1;
    case EXEC_RULE_END:
    case EXEC_ACTION_END:
    case EXEC_MICRO_SERVICE_END:
    case APPLY_RULE_END:
    case APPLY_ALL_RULES_END:
    case EXEC_MY_RULE_END:
        return 2;
    case GOT_RULE:
        return 0;
    }
    return -1;
}
/*
 * label type of rule engine event
 * flag -4 log rei
 *      otherwise do not log rei
 * action
 *
 */
int
reDebug( RuleEngineEvent label, int flag, RuleEngineEventParam *param, Node *node, Env *env, ruleExecInfo_t *rei ) {
    /* do not log anything if logging is turned off */
    if ( ruleEngineConfig.logging == 0 ) {
        return 0;
    }
    int i, m, s, status, sleepT, j;
    int processedBreakPoint = 0;
    char hdr[HEADER_TYPE_LEN];
    char *readhdr = NULL;
    char *readmsg = NULL;
    char *user = NULL;
    char *addr = NULL;
    static int mNum = 0;
    static int sNum = 0;
    static int curStat = 0;
    static int reDebugStackPtr = -1;
    static int reDebugStopAt = 1;
    char condRead[MAX_NAME_LEN];
    char myActionStr[10][MAX_NAME_LEN + 10];
    int aNum = 0;
    char seActionStr[10 * MAX_NAME_LEN + 100];
    char timestamp[TIME_LEN];
    rsComm_t *svrComm;
    int waitCnt = 0;
    sleepT = 1;
    char buf[HEADER_TYPE_LEN - 1];

    svrComm = rei->rsComm;

    if ( svrComm == NULL ) {
        rodsLog( LOG_ERROR, "Empty svrComm in REI structure for actionStr=%s\n",
                 param->actionName );
        return 0;
    }

    generateLogTimestamp( timestamp, TIME_LEN );
    printRuleEngineEventLabel( buf, HEADER_TYPE_LEN - 1, label, param );
    snprintf( hdr, HEADER_TYPE_LEN - 1,   "iaudit:%s", timestamp );
    condRead[0] = '\0';
    /* rodsLog (LOG_NOTICE,"PPP:%s\n",hdr); */
    snprintf( seActionStr, MAX_NAME_LEN + 10, "%s", buf );
    if ( GlobalREAuditFlag > 0 ) {
        if ( flag == -4 ) {
            if ( rei->uoic != NULL && strlen( rei->uoic->userName ) > 0 && strlen( rei->uoic->rodsZone ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  USER:%s@%s", rei->uoic->userName, rei->uoic->rodsZone );
                aNum++;
            }
            if ( rei->doi != NULL && strlen( rei->doi->objPath ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  DATA:%s", rei->doi->objPath );
                aNum++;
            }
            if ( rei->doi != NULL && strlen( rei->doi->filePath ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  FILE:%s", rei->doi->filePath );
                aNum++;
            }
            if ( rei->doinp != NULL && strlen( rei->doinp->objPath ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  DATAIN:%s", rei->doinp->objPath );
                aNum++;
            }
            if ( rei->doi != NULL && strlen( rei->doi->rescName ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  RESC:%s", rei->doi->rescName );
                aNum++;
            }
            if ( rei->coi != NULL && strlen( rei->coi->collName ) > 0 ) {
                snprintf( myActionStr[aNum], MAX_NAME_LEN + 10 , "  COLL:%s", rei->coi->collName );
                aNum++;
            }
            for ( j = 0; j < aNum; j++ ) {
                strncat( seActionStr, myActionStr[j], 10 * MAX_NAME_LEN + 100 - strlen( seActionStr ) );
            }
        }
    }

    /* Write audit trail */
    if ( GlobalREAuditFlag == 3 ) {
        i = _writeXMsg( GlobalREAuditFlag, hdr, seActionStr );
        if ( i < 0 ) {
            irods::log( ERROR( i, "_writeXMsg failed." ) );
        }
    }


    /* Send current position for debugging */
    if ( GlobalREDebugFlag > 5 ) {
        /* modify stack */
        int pcType = reDebugPCType( label );
        if ( ( pcType & 1 ) != 0 ) {
            pushReStack( label, param->actionName );
        }
        else if ( ( pcType & 2 ) != 0 ) {
            popReStack( label, param->actionName );
        }

        if ( curStat == REDEBUG_CONTINUE && reDebugStackCurrPtr <= reDebugStackPtr && ( reDebugPCType( label ) & reDebugStopAt ) != 0 ) {
            curStat = REDEBUG_WAIT;
        }
        if ( curStat != REDEBUG_CONTINUE ) {
            snprintf( hdr, HEADER_TYPE_LEN - 1,   "idbug:%s", param->actionName );
            i = _writeXMsg( GlobalREDebugFlag, hdr, buf );
            if ( i < 0 ) {
                irods::log( ERROR( i, "_writeXMsg failed." ) );
            }
        }

        while ( GlobalREDebugFlag > 5 ) {
            s = sNum;
            m = mNum;
            /* what should be the condition */
            sprintf( condRead, "(*XSEQNUM >= %d) && (*XADDR != \"%s:%i\") && (*XUSER  == \"%s@%s\") && ((*XHDR == \"CMSG:ALL\") %%%% (*XHDR == \"CMSG:%s:%i\"))",
                     s, myHostName, myPID, svrComm->clientUser.userName, svrComm->clientUser.rodsZone, myHostName, myPID );

            /*
            sprintf(condRead, "(*XSEQNUM >= %d)  && ((*XHDR == CMSG:ALL) %%%% (*XHDR == CMSG:%s:%i))",
            	s,  myHostName, getpid());
            */
            status = _readXMsg( GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr );
            if ( status == SYS_UNMATCHED_XMSG_TICKET ) {
                cleanUpDebug();
                return 0;
            }
            if ( status  >= 0 ) {
                rodsLog( LOG_NOTICE, "Getting XMsg:%i:%s:%s\n", s, readhdr, readmsg );
                curStat = processXMsg( GlobalREDebugFlag, readmsg,
                                       param, node, env, rei );
                if ( readhdr != NULL ) {
                    free( readhdr );
                }
                if ( readmsg != NULL ) {
                    free( readmsg );
                }
                if ( user != NULL ) {
                    free( user );
                }
                if ( addr != NULL ) {
                    free( addr );
                }

                mNum = m;
                sNum = s + 1;
                if ( curStat == REDEBUG_WAIT ) {
                    sendWaitXMsg( GlobalREDebugFlag );
                }
                else if ( curStat == REDEBUG_STEP_OVER ) {
                    reDebugStackPtr = reDebugStackCurrPtr;
                    reDebugStopAt = 1;
                    curStat = REDEBUG_CONTINUE;
                    break;
                }
                else if ( curStat == REDEBUG_STEP_OUT ) {
                    reDebugStackPtr = reDebugStackCurrPtr - 1;
                    reDebugStopAt = 2;
                    curStat = REDEBUG_CONTINUE;
                    break;
                }
                else if ( curStat == REDEBUG_STEP_CONTINUE ) {
                    reDebugStackPtr = -1;
                    curStat = REDEBUG_CONTINUE;
                    break;
                }
                else if ( curStat == REDEBUG_NEXT ) {
                    break;
                }
            }
            else {
                if ( !( curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE ) ) {

                    /*#if _POSIX_C_SOURCE >= 199309L
                    					struct timespec time = {0, 100000000}, rem;
                    					nanosleep(&time, &rem);
                    					waitCnt+=10;
                    #else*/
                    sleep( sleepT );
                    waitCnt += 100;
                    /*#endif*/
                    if ( waitCnt > 6000 ) {
                        sendWaitXMsg( GlobalREDebugFlag );
                        waitCnt = 0;
                    }
                }
            }
            if ( curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE ) {
                if ( processedBreakPoint == 1 ) {
                    break;
                }
                curStat = processBreakPoint( GlobalREDebugFlag,
                                             param, node, curStat );
                processedBreakPoint = 1;
                if ( curStat == REDEBUG_WAIT ) {
                    sendWaitXMsg( GlobalREDebugFlag );
                    continue;
                }
                else {
                    break;
                }
            }
        }
    }


    return 0;
}
