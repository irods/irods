/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reServerLib.h - header file for reServerLib.c
 */



#ifndef RE_SERVER_LIB_HPP
#define RE_SERVER_LIB_HPP

#include "rods.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "ruleExecSubmit.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.hpp"
#include "rcConnect.h"

//#include "reIn2p3SysRule.hpp"

#define DEF_NUM_RE_PROCS	1
#define RESC_UPDATE_TIME        60
#define RE_EXE	"irodsReServer"

typedef enum {
    RE_PROC_IDLE,
    RE_PROC_RUNNING,
} procExecState_t;

typedef struct {
    procExecState_t procExecState;
    ruleExecSubmitInp_t ruleExecSubmitInp;
    int status;
    int jobType;	/* 0 or RE_FAILED_STATUS */
    pid_t pid;
} reExecProc_t;

typedef struct {
    int runCnt;
    int maxRunCnt;
    int doFork;
    std::vector< reExecProc_t > reExecProc;
} reExec_t;

int
getReInfo( rcComm_t *rcComm, genQueryOut_t **genQueryOut );
int
getReInfoById( rcComm_t *rcComm, char *ruleExecId, genQueryOut_t **genQueryOut );
int
getNextQueuedRuleExec( genQueryOut_t **inGenQueryOut,
                       int startInx, ruleExecSubmitInp_t *queuedRuleExec,
                       reExec_t *reExec, int jobType );
int
regExeStatus( rcComm_t *rcComm, char *ruleExecId, char *exeStatus );
int
runQueuedRuleExec( rcComm_t *rcComm, reExec_t *reExec,
                   genQueryOut_t **genQueryOut, time_t endTime, int statusFlag );
int
initReExec( reExec_t *reExec );
int
allocReThr( reExec_t *reExec );
int
freeReThr( reExec_t *reExec, int thrInx );
int
runRuleExec( rcComm_t*, reExecProc_t *reExecProc );
int
postProcRunRuleExec( rcComm_t *rcComm, reExecProc_t *reExecProc );
int
matchRuleExecId( reExec_t *eeExec, char *ruleExecIdStr,
                 procExecState_t execState );
int
matchPidInReExec( reExec_t *reExec, pid_t pid );
int
waitAndFreeReThr( rcComm_t *rcComm, reExec_t *reExec );
int
postForkExecProc( reExecProc_t *reExecProc );
int
execRuleExec( reExecProc_t *reExecProc );
int
fillExecSubmitInp( ruleExecSubmitInp_t *ruleExecSubmitInp,  char *exeStatus,
                   char *exeTime, char *ruleExecId, char *reiFilePath, char *ruleName,
                   char *userName, char *exeAddress, char *exeFrequency, char *priority,
                   char *estimateExeTime, char *notificationAddr );
int
reServerSingleExec( char *ruleExecId, int jobType );
int
closeQueryOut( rcComm_t *conn, genQueryOut_t *genQueryOut );

#endif	/* RE_SERVER_LIB_H */
