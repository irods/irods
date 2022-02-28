/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "irods/private/re/reHelpers1.hpp"

#include "irods/private/re/reFuncDefs.hpp"
#include "irods/private/re/index.hpp"
#include "irods/private/re/parser.hpp"
#include "irods/private/re/filesystem.hpp"
#include "irods/private/re/configuration.hpp"

#include "irods/rcMisc.h"
#include "irods/irods_log.hpp"


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

    if ( svrComm == NULL ) {
        return 0;
    }
    if ( GlobalREDebugFlag != 4 ) {
        return 0;
    }

    myPID = ( int ) getpid();
    myHostName[0] = '\0';
    gethostname( myHostName, MAX_NAME_LEN );
    sprintf( condRead, "(*XUSER  == \"%s@%s\") && (*XHDR == \"STARTDEBUG\")",
             svrComm->clientUser.userName, svrComm->clientUser.rodsZone );
    return 0;
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
                snprintf( mymsg, MAX_NAME_LEN, "%s\n", param->actionName );
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
    int j;
    char hdr[HEADER_TYPE_LEN];
    static int curStat = 0;
    static int reDebugStackPtr = -1;
    static int reDebugStopAt = 1;
    char condRead[MAX_NAME_LEN];
    char myActionStr[10][MAX_NAME_LEN + 10];
    int aNum = 0;
    char seActionStr[10 * MAX_NAME_LEN + 100];
    char timestamp[TIME_LEN];
    rsComm_t *svrComm;
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
        }
    }

    return 0;
}
