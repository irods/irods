/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reServerLib.h - header file for reServerLib.c
 */



#ifndef RE_SERVER_LIB_H
#define RE_SERVER_LIB_H

#include "rods.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "ruleExecSubmit.h"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.h"
#include "reIn2p3SysRule.h"

#define MAX_RE_PROCS	4
#define DEF_NUM_RE_PROCS	1
#define RESC_UPDATE_TIME        60
#define RE_EXE	"irodsReServer"

typedef enum {
    RE_PROC_IDLE,
    RE_PROC_RUNNING,
} procExecState_t;
    
typedef struct {
    rsComm_t reComm;
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
    reExecProc_t reExecProc[MAX_RE_PROCS];
} reExec_t;

int
getReInfo (rsComm_t *rsComm, genQueryOut_t **genQueryOut);
int
getReInfoById (rsComm_t *rsComm, char *ruleExecId, genQueryOut_t **genQueryOut);
int
getNextQueuedRuleExec (rsComm_t *rsComm, genQueryOut_t **inGenQueryOut,
int startInx, ruleExecSubmitInp_t *queuedRuleExec, 
reExec_t *reExec, int jobType);
int
regExeStatus (rsComm_t *rsComm, char *ruleExecId, char *exeStatus);
int
runQueuedRuleExec (rsComm_t *rsComm, reExec_t *reExec, 
genQueryOut_t **genQueryOut, time_t endTime, int statusFlag);
int
initReExec (rsComm_t *rsComm, reExec_t *reExec);
int
allocReThr (reExec_t *reExec);
int
freeReThr (reExec_t *reExec, int thrInx);
int
runRuleExec (reExecProc_t *reExecProc);
int
postProcRunRuleExec (rsComm_t *rsComm, reExecProc_t *reExecProc);
int
matchRuleExecId (reExec_t *eeExec, char *ruleExecIdStr,
procExecState_t execState);
int
matchPidInReExec (reExec_t *reExec, pid_t pid);
int
waitAndFreeReThr (reExec_t *reExec);
int
chkAndUpdateResc (rsComm_t *rsComm);
int
postForkExecProc (rsComm_t *rsComm, reExecProc_t *reExecProc);
int
execRuleExec (reExecProc_t *reExecProc);
int
fillExecSubmitInp (ruleExecSubmitInp_t *ruleExecSubmitInp,  char *exeStatus,
char *exeTime, char *ruleExecId, char *reiFilePath, char *ruleName,
char *userName, char *exeAddress, char *exeFrequency, char *priority,
char *estimateExeTime, char *notificationAddr);
int
reServerSingleExec (rsComm_t *rsComm, char *ruleExecId, int jobType);
#endif	/* RE_SERVER_LIB_H */
