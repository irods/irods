/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsAgent.c - The main code for rodsAgent
 */

#include <syslog.h>
#include "rodsAgent.h"
#include "reconstants.h"
#include "rsApiHandler.h"
#include "icatHighLevelRoutines.h"
#include "miscServerFunct.h"
#ifdef windows_platform
#include "rsLog.h"
static void NtAgentSetEnvsFromArgs(int ac, char **av);
#endif

/* #define SERVER_DEBUG 1   */
int
main(int argc, char *argv[])
{
    int status;
    rsComm_t rsComm;
    char *tmpStr;

    ProcessType = AGENT_PT;

#ifdef RUN_SERVER_AS_ROOT
#ifndef windows_platform
    if (initServiceUser() < 0) {
        exit (1);
    }
#endif
#endif

#ifdef windows_platform
	iRODSNtAgentInit(argc, argv);
#endif

#ifndef windows_platform
    signal(SIGINT, signalExit);
    signal(SIGHUP, signalExit);
    signal(SIGTERM, signalExit);
    /* set to SIG_DFL as recommended by andy.salnikov so that system()
     * call returns real values instead of 1 */
    signal(SIGCHLD, SIG_DFL);
    signal(SIGUSR1, signalExit);
    signal(SIGPIPE, rsPipSigalHandler);
#endif

#ifndef windows_platform
#ifdef SERVER_DEBUG
    if (isPath ("/tmp/rodsdebug"))
        sleep (20);
#endif
#endif

#ifdef SYS_TIMING
    rodsLogLevel(LOG_NOTICE);
    printSysTiming ("irodsAgent", "exec", 1);
#endif

    memset (&rsComm, 0, sizeof (rsComm));

    status = initRsCommWithStartupPack (&rsComm, NULL);

    if (status < 0) {
	sendVersion (rsComm.sock, status, 0, NULL, 0);
        cleanupAndExit (status);
    }

    /* Handle option to log sql commands */
    tmpStr = getenv (SP_LOG_SQL);
    if (tmpStr != NULL) {
#ifdef IRODS_SYSLOG
       int j = atoi(tmpStr);
       rodsLogSqlReq(j);
#else
       rodsLogSqlReq(1);
#endif
    }

    /* Set the logging level */
    tmpStr = getenv (SP_LOG_LEVEL);
    if (tmpStr != NULL) {
       int i;
       i = atoi(tmpStr);
       rodsLogLevel(i);
    } else {
       rodsLogLevel(LOG_NOTICE); /* default */
    }

#ifdef IRODS_SYSLOG
/* Open a connection to syslog */
    openlog("rodsAgent",LOG_ODELAY|LOG_PID,LOG_DAEMON);
#endif

    status = getRodsEnv (&rsComm.myEnv);

    if (status < 0) {
	sendVersion (rsComm.sock, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        cleanupAndExit (status);
    }

#if RODS_CAT
    if (strstr(rsComm.myEnv.rodsDebug, "CAT") != NULL) {
       chlDebug(rsComm.myEnv.rodsDebug);
    }
#endif

#ifdef RULE_ENGINE_N
    status = initAgent (RULE_ENGINE_TRY_CACHE, &rsComm);
#else
    status = initAgent (&rsComm);
#endif
#ifdef SYS_TIMING
    printSysTiming ("irodsAgent", "initAgent", 0);
#endif

    if (status < 0) {
	sendVersion (rsComm.sock, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        cleanupAndExit (status);
    }

    /* move configConnectControl behind initAgent for now. need zoneName if
     * the user does not specify one in the input */
    initConnectControl ();

    if (rsComm.clientUser.userName[0] != '\0') {
        status = chkAllowedUser (rsComm.clientUser.userName,
         rsComm.clientUser.rodsZone);

        if (status < 0) {
            sendVersion (rsComm.sock, status, 0, NULL, 0);
            cleanupAndExit (status);
	}
    }

    /* send the server version and atatus as part of the protocol. Put
     * rsComm.reconnPort as the status */

    status = sendVersion (rsComm.sock, status, rsComm.reconnPort,
      rsComm.reconnAddr, rsComm.cookie);

    if (status < 0) {
	sendVersion (rsComm.sock, SYS_AGENT_INIT_ERR, 0, NULL, 0);
        cleanupAndExit (status);
    }
#ifdef SYS_TIMING
    printSysTiming ("irodsAgent", "sendVersion", 0);
#endif

    logAgentProc (&rsComm);

    status = agentMain (&rsComm);

    cleanupAndExit (status);

    return (status);
}

int 
agentMain (rsComm_t *rsComm)
{
    int status = 0;
    int retryCnt = 0;

    while (1) {

        if (rsComm->gsiRequest==1) {
	    status = igsiServersideAuth(rsComm) ;
	    rsComm->gsiRequest=0; 
        }
        if (rsComm->gsiRequest==2) {
	    status = ikrbServersideAuth(rsComm) ;
	    rsComm->gsiRequest=0; 
        }

	status = readAndProcClientMsg (rsComm, READ_HEADER_TIMEOUT);
#if 0
	status = readAndProcClientMsg (rsComm, 0);
#endif

	if (status >= 0) {
	    retryCnt = 0;
	    continue;
	} else {
	    if (status == DISCONN_STATUS) {
		status = 0;
		break;
	    } else {
                break;
	    }
	}
    }
    return (status);
}

