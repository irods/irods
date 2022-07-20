#ifndef IRODS_RODS_SERVER_HPP
#define IRODS_RODS_SERVER_HPP

#include "irods/rods.h"
#include "irods/rcGlobalExtern.h" // client global
#include "irods/rodsLog.h"
#include "irods/sockComm.h"
#include "irods/rsIcatOpr.hpp"
#include "irods/getRodsEnv.h"
#include "irods/rcConnect.h"

#include <unistd.h>
#include <sys/wait.h>

#include <cstdarg>
#include <csignal>
#include <vector>

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define MAX_EXEC_ENV                    10	// max number of env for execv.
#define MAX_SVR_SVR_CONNECT_CNT         7       // avoid recurive connect.

#define MIN_AGENT_TIMEOUT_TIME          7200

#define MAX_ACCEPT_ERR_CNT              1000

#define NUM_READ_WORKER_THR             5

#define RE_CACHE_SALT_NUM_RANDOM_BYTES  40

#define AGENT_QUE_CHK_INT               600     // check the agent queue every 600 sec for consistence.

// Managing the spawned agents
typedef struct agentProc {
    int pid;
    int sock;
    startupPack_t startupPack;
    struct sockaddr_in  remoteAddr;  // remote address
    struct agentProc *next;
} agentProc_t;

int serverMain(const bool enable_test_mode, const bool write_to_stdout);

int procChildren(agentProc_t** agentProcHead);

agentProc_t* getAgentProcByPid(int childPid, agentProc_t** agentProcHead);

#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(osx_platform)
void serverExit(int sig);
#else
void serverExit();
#endif

void usage(char* prog);

int initServer(rsComm_t* svrComm);

int spawnAgent(agentProc_t* connReq, agentProc_t** agentProcHead);

int execAgent(int newSock, startupPack_t* startupPack);

int getAgentProcCnt();

int chkAgentProcCnt();

int getAgentProcPIDs(std::vector<int>& _pids);

int chkConnectedAgentProcQueue();

int recordServerProcess(rsComm_t* svrComm);

int initServerMain(rsComm_t* svrComm, const bool enable_test_mode, const bool write_to_stdout);

int addConnReqToQueue(rsComm_t* rsComm, int sock);

int initConnThreadEnv();

agentProc_t* getConnReqFromQueue();

void readWorkerTask();

int procSingleConnReq(agentProc_t* connReq);

int startProcConnReqThreads();

void stopProcConnReqThreads();

void spawnManagerTask();

int procBadReq();

#endif // IRODS_RODS_SERVER_HPP

