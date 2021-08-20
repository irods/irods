#ifndef IRODS_RE_IN2P3_SYS_RULE_HPP
#define IRODS_RE_IN2P3_SYS_RULE_HPP

#include "msParam.h"
#include "rods.h"
#include "rsGlobalExtern.hpp"   // server global
#include "rcGlobalExtern.h"     // client global
#include "rsLog.hpp"
#include "rodsLog.h"
#include "sockComm.h"
#include "getRodsEnv.h"
#include "rcConnect.h"
#include "generalRowInsert.h"
#include "generalRowPurge.h"
#include "generalAdmin.h"

#include <string>

#define NFIELDS                 4       // number of fields in HostControlAccess file: <user> <group> <IP address> <subnet mask>
#define MAXLEN                  100
#define MAXSTR                  30
#define MAXLIST                 40      // max number of entries in the access list tab.

#define MON_PERF_SCRIPT         "irodsServerMonPerf"
#define MON_CFG_FILE            "../config/irodsMonPerf.config"     // contains list of servers to monitor, not mandatory.
#define NRESULT                 7                                   // number of metrics returned by MON_PERF_SCRIPT.
#define OUTPUT_MON_PERF         "../log/rodsMonPerfLog"
#define MAX_VALUE               512                                 // for array definition.
#define MAX_MESSAGE_SIZE        2000
#define MAX_NSERVERS            512                                 // max number of servers that can be monitored (load balancing).
#define TIMEOUT                 20                                  // number of seconds after which the request (the thread taking care of it) for server load is canceled.
#define MON_OUTPUT_NO_ANSWER    "#-1#-1#-1#-1#-1#-1#-1#-1#"         // used if no monitoring output from remote server.
#define LEN_SECONDS             4                                   // length in bytes for the encoding of number of seconds.

struct ThreadInput
{
    char cmd[LONG_NAME_LEN];
    char cmdArgv[HUGE_NAME_LEN];
    char execAddr[LONG_NAME_LEN];
    char hintPath[MAX_NAME_LEN];
    int threadId;
    int addPathToArgv;
    char rescName[MAX_NAME_LEN];
    ruleExecInfo_t rei;
};

using thrInp_t  = ThreadInput;

struct ResourceInfo
{
    char serverName[LONG_NAME_LEN];
    char rescName[MAX_NAME_LEN];
    char rescType[LONG_NAME_LEN];
    char vaultPath[LONG_NAME_LEN];
};

using monInfo_t = ResourceInfo;

int checkHostAccessControl(const std::string& _user_name,
                           const std::string& _client_host,
                           const std::string& _groups_name);

int msiCheckHostAccessControl(ruleExecInfo_t* rei);

int msiDigestMonStat(msParam_t* cpu_wght,
                     msParam_t* mem_wght,
                     msParam_t* swap_wght,
                     msParam_t* runq_wght,
                     msParam_t* disk_wght,
                     msParam_t* netin_wght,
                     msParam_t* netout_wght,
                     ruleExecInfo_t* rei);

int msiFlushMonStat(msParam_t* timespan, msParam_t* tablename, ruleExecInfo_t* rei);

int msiServerMonPerf(msParam_t* verbosity, msParam_t* probtime, ruleExecInfo_t* rei);

#endif // IRODS_RE_IN2P3_SYS_RULE_HPP
