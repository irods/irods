#ifndef RULE_EXEC_SUBMIT_H__
#define RULE_EXEC_SUBMIT_H__

#include "objInfo.h"

struct RcComm;
struct BytesBuf;

// definition for exeStatus
#define RE_RUNNING              "RE_RUNNING"
#define RE_IN_QUEUE             "RE_IN_QUEUE"
#define RE_FAILED               "RE_FAILED"

// definition for the statusFlag in getNextQueuedRuleExec
#define RE_FAILED_STATUS        0x1     // run the RE_FAILED too

#define REI_BUF_LEN             (100 * 1024)

#define REI_FILE_NAME           "rei"
#define DEF_REI_USER_NAME       "systemUser"
#define PACKED_REI_DIR          "packedRei"

typedef struct RuleExecSubmitInput {
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
    struct BytesBuf* packedReiAndArgBBuf;
    char ruleExecId[NAME_LEN];  // this is the output of the ruleExecSubmit
} ruleExecSubmitInp_t;

#define RULE_EXEC_SUBMIT_INP_PI "str ruleName[META_STR_LEN]; str reiFilePath[MAX_NAME_LEN]; str userName[NAME_LEN]; str exeAddress[NAME_LEN]; str exeTime[TIME_LEN]; str exeFrequency[NAME_LEN]; str priority[NAME_LEN]; str lastExecTime[NAME_LEN]; str exeStatus[NAME_LEN]; str estimateExeTime[NAME_LEN]; str notificationAddr[NAME_LEN]; struct KeyValPair_PI; struct *BytesBuf_PI; str ruleExecId[NAME_LEN];"

#ifdef __cplusplus
extern "C" {
#endif

int rcRuleExecSubmit(struct RcComm* conn, ruleExecSubmitInp_t* ruleExecSubmitInp, char** ruleExecId);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // RULE_EXEC_SUBMIT_H_

