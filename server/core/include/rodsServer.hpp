/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*-------------------------------------------------------------------------
 *
 * rodsServer.hpp-- Header file for rodsServer.cpp
 *
 *
 *
 *-------------------------------------------------------------------------
 */

#ifndef RODS_SERVER_HPP
#define RODS_SERVER_HPP

#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "rods.h"
#include "rcGlobalExtern.h"	/* client global */
#include "rodsLog.h"
#include "sockComm.h"
#include "rsIcatOpr.hpp"
#include "getRodsEnv.h"
#include "rcConnect.h"

#include <vector>

extern char *optarg;
extern int optind, opterr, optopt;
#define MAX_EXEC_ENV	10	/* max number of env for execv */
#define MAX_SVR_SVR_CONNECT_CNT 7  /* avoid recurive connect */

#define MIN_AGENT_TIMEOUT_TIME 7200

#define MAX_ACCEPT_ERR_CNT 1000

#define NUM_READ_WORKER_THR	5

#define RE_CACHE_SALT_NUM_RANDOM_BYTES 40

#define AGENT_QUE_CHK_INT	600	/* check the agent queue every 600 sec
* for consistence */

int serverMain();
int procChildren( agentProc_t **agentProcHead );
agentProc_t* getAgentProcByPid( int childPid, agentProc_t **agentProcHead );

#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(osx_platform)
void serverExit( int sig );
#else
void serverExit();
#endif

void
usage( char *prog );

int
initServer( rsComm_t *svrComm );
int
spawnAgent( agentProc_t *connReq, agentProc_t **agentProcHead );
int
execAgent( int newSock, startupPack_t *startupPack );
int
getAgentProcCnt();
int
chkAgentProcCnt();
int
getAgentProcPIDs( std::vector<int>& _pids );
int
chkConnectedAgentProcQue();
int
recordServerProcess( rsComm_t *svrComm );
int
initServerMain( rsComm_t *svrComm );
int
addConnReqToQue( rsComm_t *rsComm, int sock );
int
initConnThreadEnv();
agentProc_t *
getConnReqFromQue();
void
readWorkerTask();
int
procSingleConnReq( agentProc_t *connReq );
int
startProcConnReqThreads();
void
stopProcConnReqThreads();
void
spawnManagerTask();
int
procBadReq();
void
purgeLockFileWorkerTask();  // JMC - backport 4612
#endif	/* RODS_SERVER_H */
