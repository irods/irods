/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reServerLib.cpp - functions for the irodsReServer */

#include <sys/types.h>
#include <sys/wait.h>
#include "rcMisc.h"
#include "reServerLib.hpp"
#include "rodsConnect.h"
#include "ruleExecSubmit.h"
#include "ruleExecDel.h"
#include "genQuery.h"
#include "icatHighLevelRoutines.hpp"
#include "miscUtil.h"
#include "ruleExecMod.h"
#include "rsIcatOpr.hpp"
#include "resource.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "initServer.hpp"
#include "miscServerFunct.hpp"
#include "objMetaOpr.hpp"
#include "irods_re_structs.hpp"
#include "exec_rule_expression.h"
#include "irods_configuration_keywords.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

int closeQueryOut( rcComm_t *rsComm, genQueryOut_t *genQueryOut ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *junk = NULL;
    int status;

    if ( genQueryOut->continueInx <= 0 ) {
        return 0;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    /* maxRows = 0 specifies that the genQueryOut should be closed */
    genQueryInp.maxRows = 0;;
    genQueryInp.continueInx = genQueryOut->continueInx;

    status = rcGenQuery( rsComm, &genQueryInp, &junk );

    return status;
}

int
getReInfo( rcComm_t *rcComm, genQueryOut_t **genQueryOut ) {

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ID, ORDER_BY );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_REI_FILE_PATH, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_USER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ADDRESS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_FREQUENCY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_PRIORITY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ESTIMATED_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NOTIFICATION_ADDR, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_LAST_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_STATUS, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    *genQueryOut = NULL;
    int status =  rcGenQuery( rcComm, &genQueryInp, genQueryOut );
    clearGenQueryInp( &genQueryInp );

    if ( status < 0 ) {
        free( *genQueryOut );
        *genQueryOut = NULL;
    }
    else if ( *genQueryOut != NULL ) {
        closeQueryOut( rcComm, *genQueryOut );
    }
    return status;
}

int
getReInfoById( rcComm_t *rcComm, char *ruleExecId, genQueryOut_t **genQueryOut ) {
    genQueryInp_t genQueryInp;
    char tmpStr[NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_REI_FILE_PATH, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_USER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ADDRESS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_FREQUENCY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_PRIORITY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_ESTIMATED_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_NOTIFICATION_ADDR, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_LAST_EXE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_RULE_EXEC_STATUS, 1 );

    snprintf( tmpStr, NAME_LEN, "='%s'", ruleExecId );
    addInxVal( &genQueryInp.sqlCondInp, COL_RULE_EXEC_ID, tmpStr );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rcGenQuery( rcComm, &genQueryInp, genQueryOut );

    clearGenQueryInp( &genQueryInp );

    return status;
}

/* getNextQueuedRuleExec - get the next RuleExec in queue to run
 * jobType -   0 - run exeStatus = RE_IN_QUEUE and RE_RUNNING
 * 		  RE_FAILED_STATUS -  run the RE_FAILED too
 */

int
getNextQueuedRuleExec( genQueryOut_t **inGenQueryOut,
                       int startInx, ruleExecSubmitInp_t *queuedRuleExec,
                       reExec_t *reExec, int jobType ) {
    sqlResult_t *ruleExecId, *ruleName, *reiFilePath, *userName, *exeAddress,
                *exeTime, *exeFrequency, *priority, *lastExecTime, *exeStatus,
                *estimateExeTime, *notificationAddr;
    int i, status;
    genQueryOut_t *genQueryOut;

    if ( inGenQueryOut == NULL || *inGenQueryOut == NULL ||
            queuedRuleExec == NULL || queuedRuleExec->packedReiAndArgBBuf == NULL ||
            queuedRuleExec->packedReiAndArgBBuf->buf == NULL ) {
        rodsLog( LOG_ERROR,
                 "getNextQueuedRuleExec: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    genQueryOut = *inGenQueryOut;
    if ( ( ruleExecId = getSqlResultByInx( genQueryOut,
                                           COL_RULE_EXEC_ID ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for EXEC_ID failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( ruleName = getSqlResultByInx( genQueryOut,
                                         COL_RULE_EXEC_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for EXEC_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( reiFilePath = getSqlResultByInx( genQueryOut,
                                            COL_RULE_EXEC_REI_FILE_PATH ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for REI_FILE_PATH failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( userName = getSqlResultByInx( genQueryOut,
                                         COL_RULE_EXEC_USER_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for USER_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( exeAddress = getSqlResultByInx( genQueryOut,
                                           COL_RULE_EXEC_ADDRESS ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for EXEC_ADDRESS failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( exeTime = getSqlResultByInx( genQueryOut,
                                        COL_RULE_EXEC_TIME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for EXEC_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( exeFrequency = getSqlResultByInx( genQueryOut,
                          COL_RULE_EXEC_FREQUENCY ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec:getResultByInx for RULE_EXEC_FREQUENCY failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( priority = getSqlResultByInx( genQueryOut,
                                         COL_RULE_EXEC_PRIORITY ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for PRIORITY failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( lastExecTime = getSqlResultByInx( genQueryOut,
                          COL_RULE_EXEC_LAST_EXE_TIME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for LAST_EXE_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( exeStatus = getSqlResultByInx( genQueryOut,
                                          COL_RULE_EXEC_STATUS ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getSqlResultByInx for EXEC_STATUS failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( estimateExeTime = getSqlResultByInx( genQueryOut,
                             COL_RULE_EXEC_ESTIMATED_EXE_TIME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec: getResultByInx for ESTIMATED_EXE_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( notificationAddr = getSqlResultByInx( genQueryOut,
                              COL_RULE_EXEC_NOTIFICATION_ADDR ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getNextQueuedRuleExec:getResultByInx for NOTIFICATION_ADDR failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = startInx; i < genQueryOut->rowCnt; i++ ) {
        char *exeStatusStr, *exeTimeStr, *ruleExecIdStr;


        exeStatusStr = &exeStatus->value[exeStatus->len * i];
        exeTimeStr = &exeTime->value[exeTime->len * i];
        ruleExecIdStr =  &ruleExecId->value[ruleExecId->len * i];

        if ( ( jobType & RE_FAILED_STATUS ) == 0 &&
                strcmp( exeStatusStr, RE_FAILED ) == 0 ) {
            /* failed request */
            continue;
        }
        else if ( atoi( exeTimeStr ) > time( 0 ) ) {
            /* not time yet */
            continue;
        }
        else if ( strcmp( exeStatusStr, RE_RUNNING ) == 0 ) {
            /* is already running */
            if ( reExec->doFork == 1 &&	/* multiProc */
                    matchRuleExecId( reExec, ruleExecIdStr, RE_PROC_RUNNING ) ) {
                /* the job is running in multiProc env */
                continue;
            }
            else {
                rodsLog( LOG_NOTICE,
                         "getNextQueuedRuleExec: reId %s in RUNNING state. Run again",
                         ruleExecIdStr );
            }
        }
        status = fillExecSubmitInp( queuedRuleExec, exeStatusStr, exeTimeStr,
                                    ruleExecIdStr,
                                    &reiFilePath->value[reiFilePath->len * i],
                                    &ruleName->value[ruleName->len * i],
                                    &userName->value[userName->len * i],
                                    &exeAddress->value[exeAddress->len * i],
                                    &exeFrequency->value[exeFrequency->len * i],
                                    &priority->value[priority->len * i],
                                    &estimateExeTime->value[estimateExeTime->len * i],
                                    &notificationAddr->value[notificationAddr->len * i] );
        if ( status < 0 ) {
            continue;
        }
        return i;
    }
    return -1;
}

int
modExeInfoForRepeat( rcComm_t *rcComm, char *ruleExecId, char* pastTime,
                     char *delay, int opStatus ) {
    keyValPair_t *regParam;
    int status = 0;
    char myTimeNow[200];
    char myTimeNext[200];
    ruleExecModInp_t ruleExecModInp;
    ruleExecDelInp_t ruleExecDelInp;

    if ( opStatus > 0 ) {
        opStatus = 0;
    }

    rstrcpy( myTimeNext, pastTime, 200 );
    getOffsetTimeStr( ( char* )&myTimeNow, "                      " );

    int status1 = getNextRepeatTime( myTimeNow, delay, myTimeNext );

    /***
    if (status != 0)
      return status;
    rDelay = (atol(delay) * 60)  + atol(myTimeNow);
    sprintf(myTimeNext,"%lld", rDelay);
    ***/
    rodsLog( LOG_NOTICE, "modExeInfoForRepeat: rulId=%s,opStatus=%d,nextRepeatStatus=%d", ruleExecId, opStatus, status1 );
    regParam = &( ruleExecModInp.condInput );
    rstrcpy( ruleExecModInp.ruleId, ruleExecId, NAME_LEN );
    memset( regParam, 0, sizeof( keyValPair_t ) );
    if ( status1 == 0 ) {
        addKeyVal( regParam, RULE_EXE_STATUS_KW, "" );
        addKeyVal( regParam, RULE_LAST_EXE_TIME_KW, myTimeNow );
        addKeyVal( regParam, RULE_EXE_TIME_KW, myTimeNext );
        status = rcRuleExecMod( rcComm, &ruleExecModInp );
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "%s:%d - rcRuleExecMod failed %d", __FUNCTION__, __LINE__, status );
        }
    }
    else if ( status1 == 1 ) {
        if ( opStatus == 0 ) {
            /* entry remove  */
            rstrcpy( ruleExecDelInp.ruleExecId, ruleExecId, NAME_LEN );
            status = rcRuleExecDel( rcComm, &ruleExecDelInp );
        }
        else {
            addKeyVal( regParam, RULE_EXE_STATUS_KW, "" );
            addKeyVal( regParam, RULE_LAST_EXE_TIME_KW, myTimeNow );
            addKeyVal( regParam, RULE_EXE_TIME_KW, myTimeNext );
            status = rcRuleExecMod( rcComm, &ruleExecModInp );
            if( status < 0 ) {
                rodsLog( LOG_ERROR, "%s:%d - rcRuleExecMod failed %d", __FUNCTION__, __LINE__, status );
            }
        }
    }
    else if ( status1 == 2 ) {
        /* entry remove  */
        rstrcpy( ruleExecDelInp.ruleExecId, ruleExecId, NAME_LEN );
        status = rcRuleExecDel( rcComm, &ruleExecDelInp );
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "%s:%d - rcRuleExecDel failed %d", __FUNCTION__, __LINE__, status );
        }
    }
    else if ( status1 == 3 ) {
        addKeyVal( regParam, RULE_EXE_STATUS_KW, "" );
        addKeyVal( regParam, RULE_LAST_EXE_TIME_KW, myTimeNow );
        addKeyVal( regParam, RULE_EXE_TIME_KW, myTimeNext );
        addKeyVal( regParam, RULE_EXE_FREQUENCY_KW, delay );
        status = rcRuleExecMod( rcComm, &ruleExecModInp );
        if( status < 0 ) {
            rodsLog( LOG_ERROR, "%s:%d - rcRuleExecMod failed %d", __FUNCTION__, __LINE__, status );
        }
    }
    else if ( status1 == 4 ) {
        if ( opStatus == 0 ) {
            /* entry remove  */
            rstrcpy( ruleExecDelInp.ruleExecId, ruleExecId, NAME_LEN );
            status = rcRuleExecDel( rcComm, &ruleExecDelInp );
            if( status < 0 ) {
                rodsLog( LOG_ERROR, "%s:%d - rcRuleExecDel failed %d", __FUNCTION__, __LINE__, status );
            }
        }
        else {
            addKeyVal( regParam, RULE_EXE_STATUS_KW, "" );
            addKeyVal( regParam, RULE_LAST_EXE_TIME_KW, myTimeNow );
            addKeyVal( regParam, RULE_EXE_TIME_KW, myTimeNext );
            addKeyVal( regParam, RULE_EXE_FREQUENCY_KW, delay );
            status = rcRuleExecMod( rcComm, &ruleExecModInp );
            if( status < 0 ) {
                rodsLog( LOG_ERROR, "%s:%d - rcRuleExecMod failed %d", __FUNCTION__, __LINE__, status );
            }
        }
    }
    if ( regParam->len > 0 ) {
        clearKeyVal( regParam );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "modExeInfoForRepeat: rcRuleExecMod/rcRuleExecDel Error of id %s failed, status = %d",
                 ruleExecId, status );
    }
    else {
        if ( status1 == 3 || ( status1 != 2 && opStatus != 0 ) )
            rodsLog( LOG_NOTICE,
                     "Rule id %s set to run again at %s (frequency %s seconds)",
                     ruleExecId, myTimeNext, delay );
    }

    return status;
}

int
regExeStatus( rcComm_t *rcComm, char *ruleExecId, char *exeStatus ) {
    keyValPair_t *regParam;
    ruleExecModInp_t ruleExecModInp;
    int status;
    regParam = &( ruleExecModInp.condInput );
    memset( regParam, 0, sizeof( keyValPair_t ) );
    rstrcpy( ruleExecModInp.ruleId, ruleExecId, NAME_LEN );
    addKeyVal( regParam, RULE_EXE_STATUS_KW, exeStatus );
    status = rcRuleExecMod( rcComm, &ruleExecModInp );

    clearKeyVal( regParam );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regExeStatus: rcRuleExecMod of id %s failed, status = %d",
                 ruleExecId, status );
    }

    return status;
}

/* runQueuedRuleExec - given the job queue given in genQueryOut (from
 * a getReInfo call), run the jobs with the input jobType.
 * Valid jobType = 0 ==> normal job.
 * jobType = RE_FAILED_STATUS ==> job have failed as least once
 */

int
runQueuedRuleExec( rcComm_t *rcComm, reExec_t *reExec,
                   genQueryOut_t **genQueryOut, time_t endTime, int jobType ) {
    int inx, status;
    ruleExecSubmitInp_t *myRuleExecInp;
    int runCnt = 0;
    int thrInx;

    inx = -1;
    while ( time( NULL ) <= endTime && ( thrInx = allocReThr( reExec ) ) >= 0 ) {
        myRuleExecInp = &reExec->reExecProc[thrInx].ruleExecSubmitInp;
        if ( ( inx = getNextQueuedRuleExec( genQueryOut, inx + 1,
                                            myRuleExecInp, reExec, jobType ) ) < 0 ) {
            /* no job to run */
            freeReThr( reExec, thrInx );
            break;
        }
        else {
            reExec->reExecProc[thrInx].jobType = jobType;
        }

        /* mark running */
        status = regExeStatus( rcComm, myRuleExecInp->ruleExecId,
                               RE_RUNNING );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "runQueuedRuleExec: regExeStatus of id %s failed,stat = %d",
                     myRuleExecInp->ruleExecId, status );
            freeReThr( reExec, thrInx );
            continue;
        }

        runCnt ++;
        if ( reExec->doFork == 0 ) {
            /* single thread. Just call runRuleExec */
            status = execRuleExec( &reExec->reExecProc[thrInx] );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR, "execRuleExec failed in runQueuedRuleExec with status %d", status );
            }
            postProcRunRuleExec( rcComm, &reExec->reExecProc[thrInx] );
            freeReThr( reExec, thrInx );
            continue;
        }
        else {
            if ( ( reExec->reExecProc[thrInx].pid = fork() ) == 0 ) {
                int status = postForkExecProc( &reExec->reExecProc[thrInx] );
                exit( status );
            }
            else {
                /* parent fall through here */
                continue;
            }
        }
    }

    if ( reExec->doFork == 1 ) {
        /* wait for all jobs to finish */
        bool wait_done  = false;
        while ( !wait_done ) {
            {
                  int w = waitAndFreeReThr( rcComm, reExec );
                  if( w >= 0 ) {
                      continue;
                  }
                  else {
                      wait_done = true;
                  }
            }
        }
    }

    return runCnt;
}

int postForkExecProc( reExecProc_t * reExecProc ) {
    rodsEnv env;
    getRodsEnv(&env);
    rcComm_t* rc_comm = nullptr;
    while( !rc_comm ) {
        rc_comm = rcConnect(
                      env.rodsHost,
                      env.rodsPort,
                      env.rodsUserName,
                      env.rodsZone,
                      NO_RECONN, NULL );
        if(!rc_comm) {
            rodsLog(
                    LOG_ERROR,
                    "rcConnect failedd");
        }
    }

    int status = clientLogin( rc_comm );
    if( status < 0 ) {
        rodsLog(
                LOG_ERROR,
                "clientLogin failed %d",
                status );
        return status;
    }


    seedRandom();

    status = runRuleExec( rc_comm, reExecProc );
    postProcRunRuleExec( rc_comm, reExecProc );

    rcDisconnect( rc_comm );

    return status;
}

int
execRuleExec( reExecProc_t * reExecProc ) {

    std::vector<std::string> string_vec{
            RE_EXE,
            "-j",
            reExecProc->ruleExecSubmitInp.ruleExecId,
            "-t",
            (boost::format("%d") % reExecProc->jobType).str()
        };

    std::vector<char *> c_str_vec;
    c_str_vec.reserve( string_vec.size() + 1 );
    for (auto&& s : string_vec) {
        c_str_vec.push_back(&s[0]);
    }
    c_str_vec.push_back(nullptr);

    int status =  execv( c_str_vec[0], &c_str_vec[0] );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "execExecProc: execv of ID %s error, errno = %d",
                 reExecProc->ruleExecSubmitInp.ruleExecId, errno );
    }
    return status;
}

int
initReExec( reExec_t * reExec ) {
    int i;

    if ( reExec == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // initialize
    reExec->runCnt = 0;
    reExec->maxRunCnt = 4;
    reExec->doFork = 1;

    int max_re_procs;
    try {
        max_re_procs = irods::get_advanced_setting<const int>(irods::CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS);
    } catch ( const irods::exception& e ) {
        irods::log(e);
        return e.code();
    }

    if ( reExec->maxRunCnt > max_re_procs ) {
        reExec->maxRunCnt = max_re_procs;
    }
    reExec->doFork = 1;

    reExec->reExecProc.resize( reExec->maxRunCnt );

    for ( i = 0; i < reExec->maxRunCnt; i++ ) {
        reExec->reExecProc[i].procExecState = RE_PROC_IDLE;
        reExec->reExecProc[i].ruleExecSubmitInp.packedReiAndArgBBuf =
            ( bytesBuf_t * ) malloc( sizeof( bytesBuf_t ) );
        reExec->reExecProc[i].ruleExecSubmitInp.packedReiAndArgBBuf->buf =
            malloc( REI_BUF_LEN );
        reExec->reExecProc[i].ruleExecSubmitInp.packedReiAndArgBBuf->len =
            REI_BUF_LEN;
    }
    return 0;
}

int
allocReThr( reExec_t * reExec ) {
    int i;
    int thrInx = SYS_NO_FREE_RE_THREAD;

    if ( reExec == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( reExec->doFork == 0 ) {
        /* single thread */
        reExec->runCnt = 1;
        return 0;
    }

    for ( i = 0; i < reExec->maxRunCnt; i++ ) {
        if ( reExec->reExecProc[i].procExecState == RE_PROC_IDLE ) {
            thrInx = i;
            reExec->runCnt++;
            reExec->reExecProc[thrInx].procExecState = RE_PROC_RUNNING;
            break;
        }
    }

    return thrInx;
}

int
waitAndFreeReThr( rcComm_t * rcComm, reExec_t * reExec ) {
    pid_t childPid;
    int status = 0;
    int thrInx = SYS_NO_FREE_RE_THREAD;

    childPid = waitpid( -1, &status, WUNTRACED );
    if ( childPid < 0 ) {
        if ( reExec->runCnt > 0 ) {
            int i;
            rodsLog( LOG_NOTICE,
                     "waitAndFreeReThr: no outstanding child. but runCnt=%d",
                     reExec->runCnt );
            for ( i = 0; i < reExec->maxRunCnt; i++ ) {
                if ( reExec->reExecProc[i].procExecState != RE_PROC_IDLE ) {
                    freeReThr( reExec, i );
                }
            }
            reExec->runCnt = 0;
            thrInx = 0;
        }
    }
    else {
        thrInx = matchPidInReExec( reExec, childPid );
        if ( thrInx >= 0 ) {
            genQueryOut_t *genQueryOut = NULL;
            int status1;
            reExecProc_t *reExecProc = &reExec->reExecProc[thrInx];
            char *ruleExecId = reExecProc->ruleExecSubmitInp.ruleExecId;

            status1 = getReInfoById( rcComm, ruleExecId, &genQueryOut );
            if ( status1 >= 0 ) {
                sqlResult_t *exeFrequency, *exeStatus;
                if ( ( exeFrequency = getSqlResultByInx( genQueryOut,
                                      COL_RULE_EXEC_FREQUENCY ) ) == NULL ) {
                    rodsLog( LOG_NOTICE,
                             "waitAndFreeReThr:getResultByInx for RULE_EXEC_FREQUENCY failed" );
                }
                if ( ( exeStatus = getSqlResultByInx( genQueryOut,
                                                      COL_RULE_EXEC_STATUS ) ) == NULL ) {
                    rodsLog( LOG_NOTICE,
                             "waitAndFreeReThr:getResultByInx for RULE_EXEC_STATUS failed" );
                }

                if ( exeFrequency == NULL || exeStatus == NULL || strlen( exeFrequency->value ) == 0 || strcmp( exeStatus->value, RE_RUNNING ) == 0 ) {
                    // r5676
                    int i;
                    int overlap = 0;
                    for ( i = 0; i < reExec->maxRunCnt; i++ ) {
                        if ( i != thrInx && strcmp( reExec->reExecProc[i].ruleExecSubmitInp.ruleExecId, ruleExecId ) == 0 ) {
                            overlap++;
                        }
                    }

                    if ( overlap == 0 ) { // r5676
                        /* something wrong since the entry is not deleted. could
                         * be core dump */
                        if ( ( reExecProc->jobType & RE_FAILED_STATUS ) == 0 ) {
                            /* first time. just mark it RE_FAILED */
                            regExeStatus( rcComm, ruleExecId, RE_FAILED );
                        }
                        else {
                            ruleExecDelInp_t ruleExecDelInp;
                            rodsLog( LOG_ERROR,
                                     "waitAndFreeReThr: %s executed but still in iCat. Job deleted",
                                     ruleExecId );
                            rstrcpy( ruleExecDelInp.ruleExecId, ruleExecId, NAME_LEN );
                            status = rcRuleExecDel( rcComm, &ruleExecDelInp );
                        }
                    } // r5676
                }
                freeGenQueryOut( &genQueryOut );
            }
            freeReThr( reExec, thrInx );
        }
        // =-=-=-=-=-=-=-
    }
    return thrInx;
}

int
matchPidInReExec( reExec_t * reExec, pid_t pid ) {
    int i;

    for ( i = 0; i < reExec->maxRunCnt; i++ ) {
        if ( reExec->reExecProc[i].pid == pid ) {
            return i;
        }
    }
    rodsLog( LOG_ERROR,
             "matchPidInReExec: no match for pid %d", pid );

    return SYS_NO_FREE_RE_THREAD;
}

int
freeReThr( reExec_t * reExec, int thrInx ) {
    bytesBuf_t *packedReiAndArgBBuf;

    if ( NULL == reExec ) {
        rodsLog( LOG_ERROR, "freeReThr :: NULL reExec ptr" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if ( thrInx < 0 || thrInx >= reExec->maxRunCnt ) {
        rodsLog( LOG_ERROR, "freeReThr: Bad input thrInx %d", thrInx );
        return SYS_BAD_RE_THREAD_INX;
    }
    reExec->runCnt--;
    reExec->reExecProc[thrInx].procExecState = RE_PROC_IDLE;
    reExec->reExecProc[thrInx].status = 0;
    reExec->reExecProc[thrInx].jobType = 0;
    reExec->reExecProc[thrInx].pid = 0;
    /* save the packedReiAndArgBBuf */
    packedReiAndArgBBuf =
        reExec->reExecProc[thrInx].ruleExecSubmitInp.packedReiAndArgBBuf;

    bzero( packedReiAndArgBBuf->buf, REI_BUF_LEN );
    bzero( &reExec->reExecProc[thrInx].ruleExecSubmitInp,
           sizeof( ruleExecSubmitInp_t ) );
    reExec->reExecProc[thrInx].ruleExecSubmitInp.packedReiAndArgBBuf =
        packedReiAndArgBBuf;

    return 0;
}

int runRuleExec(
    rcComm_t*     _comm,
    reExecProc_t* _re_exec_proc ) {
    if ( !_comm || !_re_exec_proc ) {
        rodsLog(
            LOG_ERROR,
            "%s :: NULL input parameter",
            __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // unpack the rei to get the user information
    ruleExecInfoAndArg_t* rei_and_arg = NULL;
    int status = unpackStruct(
                     _re_exec_proc->ruleExecSubmitInp.packedReiAndArgBBuf->buf,
                     ( void ** ) &rei_and_arg,
                     "ReiAndArg_PI",
                     RodsPackTable,
                     NATIVE_PROT );
    if ( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "%s :: unpackStruct error. status = %d",
            __FUNCTION__,
            status );
        return status;
    }

    // set the proxy user from the rei before deleagting to the agent
    // following behavior from touchupPackedRei
    _comm->proxyUser = *rei_and_arg->rei->uoic;

    exec_rule_expression_t exec_rule;
    memset(&exec_rule,0,sizeof(exec_rule));

    int packed_rei_len = _re_exec_proc->ruleExecSubmitInp.packedReiAndArgBBuf->len;
    exec_rule.packed_rei_.len = packed_rei_len;
    exec_rule.packed_rei_.buf = _re_exec_proc->ruleExecSubmitInp.packedReiAndArgBBuf->buf;

    size_t rule_len = strlen(_re_exec_proc->ruleExecSubmitInp.ruleName);
    exec_rule.rule_text_.buf = (char*)malloc(rule_len+1);
    memset(exec_rule.rule_text_.buf,0,rule_len+1);
    exec_rule.rule_text_.len = rule_len+1;
    rstrcpy(
        (char*)exec_rule.rule_text_.buf,
        _re_exec_proc->ruleExecSubmitInp.ruleName,
        rule_len+1);
    exec_rule.params_ = rei_and_arg->rei->msParamArray;

    status = rcExecRuleExpression(
                     _comm,
                     &exec_rule );
    _re_exec_proc->status = status;

    return status;
}

int
postProcRunRuleExec( rcComm_t * rcComm, reExecProc_t * reExecProc ) {
    int status = 0;
    int savedStatus = 0;
    ruleExecDelInp_t ruleExecDelInp;
    ruleExecSubmitInp_t *myRuleExecInp;

    myRuleExecInp = &reExecProc->ruleExecSubmitInp;

    if ( strlen( myRuleExecInp->exeFrequency ) > 0 ) {
        rodsLog( LOG_NOTICE, "postProcRunRuleExec: exec of freq: %s",
                 myRuleExecInp->exeFrequency );

        savedStatus = modExeInfoForRepeat( rcComm, myRuleExecInp->ruleExecId,
                                           myRuleExecInp->exeTime, myRuleExecInp->exeFrequency,
                                           reExecProc->status );
    }
    else if ( reExecProc->status < 0 ) {
        rodsLog( LOG_ERROR,
                 "postProcRunRuleExec: ruleExec of id %s failed, status = %d",
                 myRuleExecInp->ruleExecId, reExecProc->status );
        if ( ( reExecProc->jobType & RE_FAILED_STATUS ) == 0 ) {
            /* first time. just mark it RE_FAILED */
            regExeStatus( rcComm, myRuleExecInp->ruleExecId, RE_FAILED );
        }
        else {

            /* failed once already. delete the ruleExecId */
            rodsLog( LOG_ERROR,
                     "postProcRunRuleExec: ruleExec of %s: %s failed again.Removed",
                     myRuleExecInp->ruleExecId, myRuleExecInp->ruleName );
            rstrcpy( ruleExecDelInp.ruleExecId, myRuleExecInp->ruleExecId,
                     NAME_LEN );
            status = rcRuleExecDel( rcComm, &ruleExecDelInp );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "postProcRunRuleExec: rcRuleExecDel failed for %s, stat=%d",
                         myRuleExecInp->ruleExecId, status );
            }
        }
    }
    else {
        rstrcpy( ruleExecDelInp.ruleExecId, myRuleExecInp->ruleExecId,
                 NAME_LEN );
        rodsLog( LOG_NOTICE,
                 "postProcRunRuleExec: exec of %s done", myRuleExecInp->ruleExecId );
        status = rcRuleExecDel( rcComm, &ruleExecDelInp );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "postProcRunRuleExec: rcRuleExecDel failed for %s, status = %d",
                     myRuleExecInp->ruleExecId, status );
        }
    }
    if ( status >= 0 && savedStatus < 0 ) {
        return savedStatus;
    }

    return status;
}

int
matchRuleExecId( reExec_t * reExec, char * ruleExecIdStr,
                 procExecState_t execState ) {
    int i;

    if ( reExec == NULL || ruleExecIdStr == NULL ||
            execState == RE_PROC_IDLE ) {
        return 0;
    }

    for ( i = 0; i < reExec->maxRunCnt; i++ ) {
        if ( reExec->reExecProc[i].procExecState == execState &&
                strcmp( reExec->reExecProc[i].ruleExecSubmitInp.ruleExecId,
                        ruleExecIdStr ) == 0 ) {
            return 1;
        }
    }
    return 0;
}

int
fillExecSubmitInp( ruleExecSubmitInp_t * ruleExecSubmitInp,  char * exeStatus,
                   char * exeTime, char * ruleExecId, char * reiFilePath, char * ruleName,
                   char * userName, char * exeAddress, char * exeFrequency, char * priority,
                   char * estimateExeTime, char * notificationAddr ) {
    int status;
    int fd;
    rodsLong_t st_size;

    rstrcpy( ruleExecSubmitInp->reiFilePath, reiFilePath, MAX_NAME_LEN );
    path p( ruleExecSubmitInp->reiFilePath );
    if ( !exists( p ) ) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "fillExecSubmitInp: stat error for rei file %s, id %s rule %s",
                      ruleExecSubmitInp->reiFilePath, ruleExecId, ruleName );

        return status;
    }

    st_size = file_size( p );
    if ( st_size > ruleExecSubmitInp->packedReiAndArgBBuf->len ) {
        if ( ruleExecSubmitInp->packedReiAndArgBBuf->buf != NULL ) {
            free( ruleExecSubmitInp->packedReiAndArgBBuf->buf );
        }
        ruleExecSubmitInp->packedReiAndArgBBuf->buf =
            malloc( ( int ) st_size );
        ruleExecSubmitInp->packedReiAndArgBBuf->len = st_size;
    }

    fd = open( ruleExecSubmitInp->reiFilePath, O_RDONLY, 0 );
    if ( fd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "fillExecSubmitInp: open error for rei file %s, status = %d",
                 ruleExecSubmitInp->reiFilePath, status );
        return status;
    }

    bzero( ruleExecSubmitInp->packedReiAndArgBBuf->buf,
           ruleExecSubmitInp->packedReiAndArgBBuf->len );
    status = read( fd, ruleExecSubmitInp->packedReiAndArgBBuf->buf,
                   ruleExecSubmitInp->packedReiAndArgBBuf->len );
    close( fd );
    if ( status != ( int ) st_size ) {
        if ( status < 0 ) {
            status = UNIX_FILE_READ_ERR - errno;
            rodsLog( LOG_ERROR,
                     "fillExecSubmitInp: read error for file %s, status = %d",
                     ruleExecSubmitInp->reiFilePath, status );
        }
        else {
            rodsLog( LOG_ERROR,
                     "fillExecSubmitInp:read error for %s,toRead %d, read %d",
                     ruleExecSubmitInp->reiFilePath,
                     ruleExecSubmitInp->packedReiAndArgBBuf->len, status );
            return SYS_COPY_LEN_ERR;
        }
    }

    rstrcpy( ruleExecSubmitInp->exeTime, exeTime, TIME_LEN );
    rstrcpy( ruleExecSubmitInp->exeStatus, exeStatus, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->ruleExecId, ruleExecId, NAME_LEN );

    rstrcpy( ruleExecSubmitInp->ruleName, ruleName, META_STR_LEN );
    rstrcpy( ruleExecSubmitInp->userName, userName, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->exeAddress, exeAddress, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->exeFrequency, exeFrequency, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->priority, priority, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->estimateExeTime, estimateExeTime, NAME_LEN );
    rstrcpy( ruleExecSubmitInp->notificationAddr, notificationAddr, NAME_LEN );

    return 0;
}

int
reServerSingleExec( char * ruleExecId, int jobType ) {
    rodsEnv env;
    getRodsEnv(&env);

    rcComm_t* rc_comm = nullptr;
    while( !rc_comm ) {
        rc_comm = rcConnect(
                env.rodsHost,
                env.rodsPort,
                env.rodsUserName,
                env.rodsZone,
                NO_RECONN, NULL );
        if(!rc_comm) {
            rodsLog(
                    LOG_ERROR,
                    "rcConnect failed %d");
        }
    }

    int status = clientLogin( rc_comm );
    if( status < 0 ) {
        rodsLog(
            LOG_ERROR,
            "clientLogin failed %d",
            status );
        return status;
    }

    genQueryOut_t *genQueryOut = NULL;
    status = getReInfoById( rc_comm, ruleExecId, &genQueryOut );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "reServerSingleExec: getReInfoById error for %s, status = %d",
                 ruleExecId, status );
        rcDisconnect( rc_comm );
        return status;
    }

    std::unique_ptr<sqlResult_t> ruleName(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_NAME ));
    if ( ruleName == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for EXEC_NAME failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> reiFilePath(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_REI_FILE_PATH ));
    if ( reiFilePath == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for REI_FILE_PATH failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> userName(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_USER_NAME ));
    if ( userName == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for USER_NAME failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> exeAddress(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_ADDRESS ));
    if ( exeAddress == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for EXEC_ADDRESS failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> exeTime(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_TIME ));
    if ( exeTime == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for EXEC_TIME failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> exeFrequency(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_FREQUENCY ));
    if ( exeFrequency == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec:getResultByInx for RULE_EXEC_FREQUENCY failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> priority(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_PRIORITY ));
    if ( priority == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for PRIORITY failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> cTimelastExe(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_LAST_EXE_TIME ));
    if ( cTimelastExe == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for LAST_EXE_TIME failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> exeStatus(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_STATUS ));
    if ( exeStatus == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getSqlResultByInx for EXEC_STATUS failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> estimateExeTime(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_ESTIMATED_EXE_TIME ));
    if ( estimateExeTime == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec: getResultByInx for ESTIMATED_EXE_TIME failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }
    std::unique_ptr<sqlResult_t> notificationAddr(getSqlResultByInx( genQueryOut, COL_RULE_EXEC_NOTIFICATION_ADDR ));
    if ( notificationAddr == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "reServerSingleExec:getResultByInx for NOTIFICATION_ADDR failed" );
        rcDisconnect( rc_comm );
        return UNMATCHED_KEY_OR_INDEX;
    }

    reExecProc_t reExecProc;
    bzero( &reExecProc, sizeof( reExecProc ) );
    /* init reComm */
    bytesBuf_t reBuf;
    memset(&reBuf, 0, sizeof(reBuf));
    reExecProc.ruleExecSubmitInp.packedReiAndArgBBuf = &reBuf;
    reExecProc.procExecState = RE_PROC_RUNNING;
    reExecProc.jobType = jobType;
    status = fillExecSubmitInp( &reExecProc.ruleExecSubmitInp,
                                exeStatus->value, exeTime->value, ruleExecId, reiFilePath->value,
                                ruleName->value, userName->value, exeAddress->value, exeFrequency->value,
                                priority->value, estimateExeTime->value, notificationAddr->value );

    if ( status < 0 ) {
        if ( reExecProc.ruleExecSubmitInp.packedReiAndArgBBuf->buf ) {
            free(reExecProc.ruleExecSubmitInp.packedReiAndArgBBuf->buf);
        }
        rcDisconnect( rc_comm );
        return status;
    }
    seedRandom();
    status = runRuleExec( rc_comm, &reExecProc );
    postProcRunRuleExec( rc_comm, &reExecProc );

    if ( reExecProc.ruleExecSubmitInp.packedReiAndArgBBuf->buf ) {
        free(reExecProc.ruleExecSubmitInp.packedReiAndArgBBuf->buf);
    }
    freeGenQueryOut( &genQueryOut );

    rcDisconnect( rc_comm );

    return status;
}
