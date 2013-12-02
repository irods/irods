/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ruleExecSubmit.h
 */

#ifndef RULE_EXEC_SUBMIT_H
#define RULE_EXEC_SUBMIT_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "reGlobalsExtern.h"

/* definition for exeStatus */
#define RE_RUNNING	"RE_RUNNING"
#define RE_IN_QUEUE	"RE_IN_QUEUE"
#define RE_FAILED	"RE_FAILED"
 
/* definition for the statusFlag in getNextQueuedRuleExec */
#define RE_FAILED_STATUS	0x1	/* run the RE_FAILED too */ 
 
#define REI_BUF_LEN             (100 * 1024)

#define REI_FILE_NAME	"rei"
#define DEF_REI_USER_NAME	"systemUser"
#define PACKED_REI_DIR		"packedRei"
typedef struct {
    char ruleName[META_STR_LEN];
    char reiFilePath[MAX_NAME_LEN];
    char userName[NAME_LEN];
    char exeAddress[NAME_LEN];
    char exeTime[TIME_LEN];
    char exeFrequency[NAME_LEN];
    char priority[NAME_LEN];
    char lastExecTime[NAME_LEN];
    char exeStatus[NAME_LEN];
    char estimateExeTime[NAME_LEN];
    char notificationAddr[NAME_LEN];
    keyValPair_t condInput;
    bytesBuf_t *packedReiAndArgBBuf;
    char ruleExecId[NAME_LEN];	/* this is the output of the ruleExecSubmit */
} ruleExecSubmitInp_t;

#define RULE_EXEC_SUBMIT_INP_PI "str ruleName[META_STR_LEN]; str reiFilePath[MAX_NAME_LEN]; str userName[NAME_LEN]; str exeAddress[NAME_LEN]; str exeTime[TIME_LEN]; str exeFrequency[NAME_LEN]; str priority[NAME_LEN]; str lastExecTime[NAME_LEN]; str exeStatus[NAME_LEN]; str estimateExeTime[NAME_LEN]; str notificationAddr[NAME_LEN]; struct KeyValPair_PI; struct *BytesBuf_PI; str ruleExecId[NAME_LEN];"

#if defined(RODS_SERVER)
#define RS_RULE_EXEC_SUBMIT rsRuleExecSubmit
/* prototype for the server handler */
int
rsRuleExecSubmit (rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp,
char **ruleExecId);
int
_rsRuleExecSubmit (rsComm_t *rsComm, ruleExecSubmitInp_t *ruleExecSubmitInp);
#else
#define RS_RULE_EXEC_SUBMIT NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

int
rcRuleExecSubmit (rcComm_t *conn, ruleExecSubmitInp_t *ruleExecSubmitInp,
char **ruleExecId);

int 
getReiFilePath (char *reiFilePath, char *userName);
#ifdef  __cplusplus
}
#endif

#endif	/* RULE_EXEC_SUBMIT_H */
