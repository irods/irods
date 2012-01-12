/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rodsServer.h"
#include "resource.h"
#include "miscServerFunct.h"

#include <syslog.h>

#ifndef SINGLE_SVR_THR
#include <pthread.h>
#endif
#ifndef windows_platform
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef windows_platform
#include "irodsntutil.h"
#endif

uint ServerBootTime;
int SvrSock;

agentProc_t *ConnectedAgentHead = NULL;
agentProc_t *ConnReqHead = NULL;
agentProc_t *SpawnReqHead = NULL;
agentProc_t *BadReqHead = NULL;

#if 0	/* defined in config.mk */
#define USE_BOOST 
#define USE_BOOST_COND
#endif

#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST
	#include <boost/thread/thread.hpp>
	#include <boost/thread/mutex.hpp>
	#include <boost/thread/condition.hpp>
	boost::mutex		  ConnectedAgentMutex;
	boost::mutex		  BadReqMutex;
	boost::thread*		  ReadWorkerThread[NUM_READ_WORKER_THR];
	boost::thread*		  SpawnManagerThread;
	#else
	pthread_mutex_t ConnectedAgentMutex;
	pthread_mutex_t BadReqMutex;
	pthread_t       ReadWorkerThread[NUM_READ_WORKER_THR];
	pthread_t       SpawnManagerThread;
	#endif
#endif

#ifdef USE_BOOST_COND
	boost::mutex		  ReadReqCondMutex;
	boost::mutex		  SpawnReqCondMutex;
	boost::condition_variable ReadReqCond;
	boost::condition_variable SpawnReqCond;
#else
	pthread_mutex_t ReadReqCondMutex;
	pthread_mutex_t SpawnReqCondMutex;
	pthread_cond_t ReadReqCond;
	pthread_cond_t SpawnReqCond;
#endif

#ifndef windows_platform   /* all UNIX */
int
main(int argc, char **argv)
#else   /* Windows */
int irodsWinMain(int argc, char **argv)
#endif
{
    int c;
    int uFlag = 0;
    char tmpStr1[100], tmpStr2[100];
    char *logDir = NULL;
    char *tmpStr;


    ProcessType = SERVER_PT;	/* I am a server */

#ifdef RUN_SERVER_AS_ROOT
#ifndef windows_platform
    if (initServiceUser() < 0) {
        exit (1);
    }
#endif
#endif

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
	openlog("rodsServer",LOG_ODELAY|LOG_PID,LOG_DAEMON);
#endif
    
    ServerBootTime = time (0);
    while ((c = getopt(argc, argv,"uvVqsh")) != EOF) {
        switch (c) {
            case 'u':		/* user command level. without serverized */
		uFlag = 1;
                break;
            case 'D':   /* user specified a log directory */
                logDir = strdup (optarg);
                break;
	    case 'v':		/* verbose Logging */
		snprintf(tmpStr1,100,"%s=%d",SP_LOG_LEVEL, LOG_NOTICE);
		putenv(tmpStr1);
	        rodsLogLevel(LOG_NOTICE);
                break;
  	    case 'V':		/* very Verbose */
		snprintf(tmpStr1,100,"%s=%d",SP_LOG_LEVEL, LOG_DEBUG1);
		putenv(tmpStr1);
	        rodsLogLevel(LOG_DEBUG1);
                break;
	    case 'q':           /* quiet (only errors and above) */
		snprintf(tmpStr1,100,"%s=%d",SP_LOG_LEVEL, LOG_ERROR);
		putenv(tmpStr1);
	        rodsLogLevel(LOG_ERROR);
                break;
  	    case 's':		/* log SQL commands */
		snprintf(tmpStr2,100,"%s=%d",SP_LOG_SQL, 1);
		putenv(tmpStr2);
                break;
	    case 'h':		/* help */
                usage (argv[0]);
                exit (0);
            default:
                usage (argv[0]);
                exit (1);
        }
    }

    if (uFlag == 0) {
	if (serverize (logDir) < 0) {
	    exit (1);
	}
    }

    /* start of irodsReServer has been moved to serverMain */
#ifndef _WIN32
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_DFL);	/* SIG_IGN causes autoreap. wait get nothing */
    signal(SIGPIPE, SIG_IGN);
#ifdef osx_platform
#if 0
    signal(SIGINT, (void *) serverExit);
    signal(SIGHUP, (void *)serverExit);
    signal(SIGTERM, (void *)serverExit);
#else
    signal(SIGINT, (sig_t) serverExit);
    signal(SIGHUP, (sig_t)serverExit);
    signal(SIGTERM, (sig_t) serverExit);
#endif
#else
    signal(SIGINT, serverExit);
    signal(SIGHUP, serverExit);
    signal(SIGTERM, serverExit);
#endif
#endif
/* log the server timeout */

#ifndef _WIN32
    /* Initialize ServerTimeOut */
    if (getenv ("ServerTimeOut") != NULL) {
        int serverTimeOut;
        serverTimeOut = atoi (getenv ("ServerTimeOut"));
        if (serverTimeOut < MIN_AGENT_TIMEOUT_TIME) {
            rodsLog (LOG_NOTICE,
             "ServerTimeOut %d is less than min of %d",
             serverTimeOut, MIN_AGENT_TIMEOUT_TIME);
            serverTimeOut = MIN_AGENT_TIMEOUT_TIME;
        }
        rodsLog (LOG_NOTICE,
         "ServerTimeOut has been set to %d seconds",
         serverTimeOut);
    }
#endif

    serverMain (logDir);
    exit (0);
}

int 
serverize (char *logDir)
{
    char *logFile = NULL;

#ifdef windows_platform
	if(iRODSNtServerRunningConsoleMode())
		return;
#endif

    getLogfileName (&logFile, logDir, RODS_LOGFILE);

#ifndef windows_platform
#ifdef IRODS_SYSLOG
    LogFd = 0;
#else
    LogFd = open (logFile, O_CREAT|O_WRONLY|O_APPEND, 0644);
#endif
#else
    LogFd = iRODSNt_open(logFile, O_CREAT|O_APPEND|O_WRONLY, 1);
#endif

    if (LogFd < 0) {
        rodsLog (LOG_NOTICE, "logFileOpen: Unable to open %s. errno = %d",
          logFile, errno);
        return (-1);
    }

#ifndef windows_platform
    if (fork()) {	/* parent */
        exit (0);
    } else {	/* child */
        if (setsid() < 0) {
            rodsLog (LOG_NOTICE, 
	     "serverize: setsid failed, errno = %d\n", errno);
            exit(1);
	}
#ifndef IRODS_SYSLOG
        (void) dup2 (LogFd, 0);
        (void) dup2 (LogFd, 1);
        (void) dup2 (LogFd, 2);
        close (LogFd);
        LogFd = 2;
#endif
    }
#else
	_close(LogFd);
#endif

#ifdef IRODS_SYSLOG
    return (0);
#else
    return (LogFd);
#endif
}

int 
serverMain (char *logDir)
{
    int status;
    rsComm_t svrComm;
    fd_set sockMask;
    int numSock;
    int newSock;
    int loopCnt = 0;
    int acceptErrCnt = 0;
#ifdef SYS_TIMING
    int connCnt = 0;
#endif


    status = initServerMain (&svrComm);
    if (status < 0) {
        rodsLog (LOG_ERROR, "serverMain: initServerMain error. status = %d",
          status);
        exit (1);
    }
#ifndef SINGLE_SVR_THR
    startProcConnReqThreads ();
#endif
    FD_ZERO(&sockMask);

    SvrSock = svrComm.sock;
    while (1) {		/* infinite loop */
        FD_SET(svrComm.sock, &sockMask);
        while ((numSock = select (svrComm.sock + 1, &sockMask, 
	  (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL)) < 0) {

            if (errno == EINTR) {
		rodsLog (LOG_NOTICE, "serverMain: select() interrupted");
                FD_SET(svrComm.sock, &sockMask);
		continue;
	    } else {
		rodsLog (LOG_NOTICE, "serverMain: select() error, errno = %d",
		  errno);
	        return (-1);
	    }
	}

	procChildren (&ConnectedAgentHead);
#ifdef SYS_TIMING
	initSysTiming ("irodsServer", "recv connection", 0);
#endif

	newSock = rsAcceptConn (&svrComm);

	if (newSock < 0) {
	    acceptErrCnt ++;
	    if (acceptErrCnt > MAX_ACCEPT_ERR_CNT) {
	        rodsLog (LOG_ERROR, 
		  "serverMain: Too many socket accept error. Exiting");
		break;
	    } else {
	        rodsLog (LOG_NOTICE, 
	          "serverMain: acceptConn () error, errno = %d", errno);
	        continue;
	    }
	} else {
	    acceptErrCnt = 0;
	}

	status = chkAgentProcCnt ();
	if (status < 0) {
            rodsLog (LOG_NOTICE, 
	      "serverMain: chkAgentProcCnt failed status = %d", status);
            sendVersion (newSock, status, 0, NULL, 0);
	    status = mySockClose (newSock);
            printf ("close status = %d\n", status);
	    continue;
        }

	addConnReqToQue (&svrComm, newSock);

#ifdef SYS_TIMING
        connCnt ++;
        rodsLog (LOG_NOTICE, "irodsServer: agent proc count = %d, connCnt = %d",
          getAgentProcCnt (ConnectedAgentHead), connCnt);
#endif

#ifdef SINGLE_SVR_THR
	procSingleConnReq (ConnReqHead);
	ConnReqHead = NULL;
#endif

#ifndef windows_platform
        loopCnt++;
        if (loopCnt >= LOGFILE_CHK_CNT) {
            chkLogfileName (logDir, RODS_LOGFILE);
	    loopCnt = 0;
        }
#endif
    }		/* infinite loop */

    /* not reached - return (status); */
    return(0); /* to avoid warning */
}

void
#if defined(linux_platform) || defined(aix_platform) || defined(solaris_platform) || defined(linux_platform) || defined(osx_platform)
serverExit (int sig)
#else
serverExit ()
#endif
{
#ifndef windows_platform
    rodsLog (LOG_NOTICE, "rodsServer caught signal %d, exiting", sig);
#else
	rodsLog (LOG_NOTICE, "rodsServer is exiting.");
#endif
    recordServerProcess(NULL); /* unlink the process id file */
    exit (1);
}

void
usage (char *prog)
{
    printf ("Usage: %s [-uvVqs]\n", prog);
    printf (" -u  user command level, remain attached to the tty\n");
    printf (" -v  verbose (LOG_NOTICE)\n");
    printf (" -V  very verbose (LOG_DEBUG1)\n");
    printf (" -q  quiet (LOG_ERROR)\n");
    printf (" -s  log SQL commands\n");

}

int
procChildren (agentProc_t **agentProcHead)
{
    int childPid;
    agentProc_t *tmpAgentProc;
    int status;


#ifndef _WIN32
    while ((childPid = waitpid (-1, &status, WNOHANG | WUNTRACED)) > 0) {
	tmpAgentProc = getAgentProcByPid (childPid, agentProcHead);
	if (tmpAgentProc != NULL) {
	    rodsLog (LOG_NOTICE, "Agent process %d exited with status %d", 
	      childPid, status);
	    free (tmpAgentProc);
	} else {
	    rodsLog (LOG_NOTICE, 
	      "Agent process %d exited with status %d but not in queue",
	      childPid, status); 
	}
	rmProcLog (childPid);
    }
#endif

    return (0);
}


agentProc_t *
getAgentProcByPid (int childPid, agentProc_t **agentProcHead) 
{
    agentProc_t *tmpAgentProc, *prevAgentProc;
    prevAgentProc = NULL;
 
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST
	boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
	#else
	pthread_mutex_lock(&ConnectedAgentMutex);
	#endif
#endif
    tmpAgentProc = *agentProcHead;

    while (tmpAgentProc != NULL) {
	if (childPid == tmpAgentProc->pid) {
	    if (prevAgentProc == NULL) {
		*agentProcHead = tmpAgentProc->next;
	    } else {
		prevAgentProc->next = tmpAgentProc->next;
	    }
	    break;
	}
	prevAgentProc = tmpAgentProc;
	tmpAgentProc = tmpAgentProc->next;
    }
#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    con_agent_lock.unlock();
    #else
    pthread_mutex_unlock (&ConnectedAgentMutex);
    #endif
#endif
    return (tmpAgentProc);
}


int
spawnAgent (agentProc_t *connReq, agentProc_t **agentProcHead)
{
    int childPid;
    int newSock;
    startupPack_t *startupPack;

    if (connReq == NULL) return USER__NULL_INPUT_ERR;

    newSock = connReq->sock;
    startupPack = &connReq->startupPack;

#ifndef windows_platform
    childPid = fork ();	/* use fork instead of vfork because of multi-thread
			 * env */

    if (childPid < 0) {
	return SYS_FORK_ERROR -errno;
    } else if (childPid == 0) {	/* child */
	agentProc_t *tmpAgentProc;
	close (SvrSock);
#ifdef SYS_TIMING
        printSysTiming ("irodsAent", "after fork", 0);
        initSysTiming ("irodsAent", "after fork", 1);
#endif
	/* close any socket still in the queue */
#ifndef SINGLE_SVR_THR
	/* These queues may be inconsistent because of the multi-threading 
         * of the parent. set sock to -1 if it has been closed */
	tmpAgentProc = ConnReqHead;
	while (tmpAgentProc != NULL) {
	    if (tmpAgentProc->sock == -1) break;
	    close (tmpAgentProc->sock);
	    tmpAgentProc->sock = -1;
	    tmpAgentProc = tmpAgentProc->next;
	}
        tmpAgentProc = SpawnReqHead;
        while (tmpAgentProc != NULL) {
	    if (tmpAgentProc->sock == -1) break;
            close (tmpAgentProc->sock);
	    tmpAgentProc->sock = -1;
            tmpAgentProc = tmpAgentProc->next;
        }
#endif
	execAgent (newSock, startupPack);
    } else {			/* parent */
#ifdef SYS_TIMING
	printSysTiming ("irodsServer", "fork agent", 0);
#endif
	queConnectedAgentProc (childPid, connReq, agentProcHead);
    }
#else
	childPid = execAgent (newSock, startupPack);
	queConnectedAgentProc (childPid, connReq, agentProcHead);
#endif

    return (childPid);
}

int
execAgent (int newSock, startupPack_t *startupPack)
{
#if windows_platform
	char *myArgv[3];
	char console_buf[20];
#else
    char *myArgv[2];
#endif
    int status;
    char buf[NAME_LEN];

    mySetenvInt (SP_NEW_SOCK, newSock);
    mySetenvInt (SP_PROTOCOL, startupPack->irodsProt);
    mySetenvInt (SP_RECONN_FLAG, startupPack->reconnFlag);
    mySetenvInt (SP_CONNECT_CNT, startupPack->connectCnt);
    mySetenvStr (SP_PROXY_USER, startupPack->proxyUser);
    mySetenvStr (SP_PROXY_RODS_ZONE, startupPack->proxyRodsZone);
    mySetenvStr (SP_CLIENT_USER, startupPack->clientUser);
    mySetenvStr (SP_CLIENT_RODS_ZONE, startupPack->clientRodsZone);
    mySetenvStr (SP_REL_VERSION, startupPack->relVersion);
    mySetenvStr (SP_API_VERSION, startupPack->apiVersion);
    mySetenvStr (SP_OPTION, startupPack->option);
    mySetenvInt (SERVER_BOOT_TIME, ServerBootTime);

#if 0
    char *myBuf;
    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%d", SP_NEW_SOCK, newSock);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%d", SP_PROTOCOL,
      startupPack->irodsProt);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%d", SP_RECONN_FLAG,
      startupPack->reconnFlag);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%d", SP_CONNECT_CNT,
      startupPack->connectCnt);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_PROXY_USER,
      startupPack->proxyUser);
    putenv (myBuf);

	 myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_PROXY_RODS_ZONE,
      startupPack->proxyRodsZone);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_CLIENT_USER,
      startupPack->clientUser);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_CLIENT_RODS_ZONE,
      startupPack->clientRodsZone);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_REL_VERSION,
      startupPack->relVersion);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_API_VERSION,
      startupPack->apiVersion);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%s", SP_OPTION,
      startupPack->option);
    putenv (myBuf);

    myBuf = malloc (NAME_LEN * 2);
    snprintf (myBuf, NAME_LEN * 2, "%s=%d", SERVER_BOOT_TIME,
      ServerBootTime);
    putenv (myBuf);
#endif

#ifdef windows_platform  /* windows */
	iRODSNtGetAgentExecutableWithPath(buf, AGENT_EXE);
	myArgv[0] = buf;
	if(iRODSNtServerRunningConsoleMode())
	{
		strcpy(console_buf, "console");
		myArgv[1]= console_buf;
		myArgv[2] = NULL;
	}
	else
	{
		myArgv[1] = NULL;
		myArgv[2] = NULL;
	}
#else
    rstrcpy (buf, AGENT_EXE, NAME_LEN);
    myArgv[0] = buf;
    myArgv[1] = NULL;
#endif

#if windows_platform  /* windows */
	return (int)_spawnv(_P_NOWAIT,myArgv[0], myArgv);
#else
    status = execv(myArgv[0], myArgv);
    if (status < 0) {
        rodsLog (LOG_ERROR, "execAgent: execv error errno=%d", errno);
	exit (1);
    }
    return (0);
#endif
}

int
queConnectedAgentProc (int childPid, agentProc_t *connReq, 
agentProc_t **agentProcHead)
{
    if (connReq == NULL) 
        return USER__NULL_INPUT_ERR;

    connReq->pid = childPid;
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST
	boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
	#else
	pthread_mutex_lock (&ConnectedAgentMutex);
	#endif
#endif

    queAgentProc (connReq, agentProcHead, TOP_POS);

#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    con_agent_lock.unlock();
    #else
    pthread_mutex_unlock (&ConnectedAgentMutex);
    #endif
#endif

    return (0);
}

int
getAgentProcCnt ()
{
    agentProc_t *tmpAagentProc;
    int count = 0;

#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
    #else
    pthread_mutex_lock (&ConnectedAgentMutex);
    #endif
#endif

    tmpAagentProc = ConnectedAgentHead;
    while (tmpAagentProc != NULL) {
	count++;
	tmpAagentProc = tmpAagentProc->next;
    }
#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    con_agent_lock.unlock();
    #else
    pthread_mutex_unlock (&ConnectedAgentMutex);
    #endif
#endif

    return count;
}

int
chkAgentProcCnt ()
{
    int count; 

    if (MaxConnections == NO_MAX_CONNECTION_LIMIT) return 0;
    count = getAgentProcCnt ();
    if (count >= MaxConnections) {
	chkConnectedAgentProcQue ();
	count = getAgentProcCnt ();
	if (count >= MaxConnections) {
	    return SYS_MAX_CONNECT_COUNT_EXCEEDED;
	} else {
            return 0;
        }
    } else {
	return 0;
    }
}
	
int
chkConnectedAgentProcQue ()
{
    agentProc_t *tmpAgentProc, *prevAgentProc, *unmatchedAgentProc;
    prevAgentProc = NULL;

#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST
	boost::unique_lock< boost::mutex > con_agent_lock( ConnectedAgentMutex );
	#else
	pthread_mutex_lock (&ConnectedAgentMutex);
	#endif
#endif
    tmpAgentProc = ConnectedAgentHead;

    while (tmpAgentProc != NULL) {
        char procPath[MAX_NAME_LEN];
#ifndef USE_BOOST_FS
	struct stat statbuf;
#endif

        snprintf (procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, 
	  tmpAgentProc->pid);
#ifdef USE_BOOST_FS
        path p (procPath);
        if (!exists (p)) {
#else
	if (stat (procPath, &statbuf) < 0) {
#endif
	    /* the agent proc is gone */
	    unmatchedAgentProc = tmpAgentProc;
	    rodsLog (LOG_DEBUG, 
	      "Agent process %d in Connected queue but not in ProcLogDir",
              tmpAgentProc->pid);
            if (prevAgentProc == NULL) {
                ConnectedAgentHead = tmpAgentProc->next;
            } else {
                prevAgentProc->next = tmpAgentProc->next;
            }
	    tmpAgentProc = tmpAgentProc->next;
	    free (unmatchedAgentProc);
        } else {
            prevAgentProc = tmpAgentProc;
            tmpAgentProc = tmpAgentProc->next;
	}
    }
#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    con_agent_lock.unlock();
    #else
    pthread_mutex_unlock (&ConnectedAgentMutex);
    #endif
#endif
    return 0;
}

int
initServer ( rsComm_t *svrComm)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

#ifdef windows_platform
	status = startWinsock();
	if(status !=0)
	{
		rodsLog (LOG_NOTICE, "initServer: startWinsock() failed. status=%d", status);
		return -1;
	}
#endif

    status = initServerInfo (svrComm);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "initServer: initServerInfo error, status = %d",
          status);
        return (status);
    }

    printLocalResc ();

    printZoneInfo ();

    status = getRcatHost (MASTER_RCAT, NULL, &rodsServerHost);

    if (status < 0) {
        return (status);
    }

    if (rodsServerHost->localFlag == LOCAL_HOST) {
#if RODS_CAT
        disconnectRcat (svrComm);
#endif
    } else {
	if (rodsServerHost->conn != NULL) {
	    rcDisconnect (rodsServerHost->conn);
	    rodsServerHost->conn = NULL;
	}
    } 
    initConnectControl ();

    return (status);
}

/* record the server process number and other information into
   a well-known file.  If svrComm is Null and this has created a file
   before, just unlink the file. */
int
recordServerProcess(rsComm_t *svrComm) {
#ifndef windows_platform
	int myPid;
    FILE *fd;
    DIR  *dirp;
    static char filePath[100]="";
    char cwd[1000];
    char stateFile[]="irodsServer";
    char *tmp;
    char *cp;

	 if (svrComm == NULL) {
		 if (filePath[0]!='\0') {
			 unlink(filePath);
		 }
		 return 0;
	 }
    rodsEnv *myEnv = &(svrComm->myEnv);
    
    /* Use /usr/tmp if it exists, /tmp otherwise */
    dirp = opendir("/usr/tmp");
    if (dirp!=NULL) {
       tmp="/usr/tmp";
       (void)closedir( dirp );
    }
    else {
       tmp="/tmp";
    }

    sprintf (filePath, "%s/%s.%d", tmp, stateFile, myEnv->rodsPort);

    unlink(filePath);

    myPid=getpid();
    cp = getcwd(cwd, 1000);
    if (cp != NULL) {
       fd = fopen (filePath, "w");
       if (fd != NULL) {
	  fprintf(fd, "%d %s\n", myPid, cwd);
	  fclose(fd);
	  chmod(filePath, 0664);
       }
    }
#endif
    return 0;
}

int
initServerMain (rsComm_t *svrComm)
{
    int status;
    rodsServerHost_t *reServerHost = NULL;
    rodsServerHost_t *xmsgServerHost = NULL;

    bzero (svrComm, sizeof (rsComm_t));

    status = getRodsEnv (&svrComm->myEnv);

    if (status < 0) {
        rodsLog (LOG_ERROR, "initServerMain: getRodsEnv error. status = %d",
          status);
        return status;
    }
    initAndClearProcLog ();

    setRsCommFromRodsEnv (svrComm);

    status = initServer (svrComm);

    if (status < 0) {
        rodsLog (LOG_ERROR, "initServerMain: initServer error. status = %d",
          status);
        exit (1);
    }
    svrComm->sock = sockOpenForInConn (svrComm, &svrComm->myEnv.rodsPort,
      NULL, SOCK_STREAM);

    if (svrComm->sock < 0) {
        rodsLog (LOG_ERROR,
          "initServerMain: sockOpenForInConn error. status = %d",
          svrComm->sock);
        return svrComm->sock;
    }

    listen (svrComm->sock, MAX_LISTEN_QUE);

    rodsLog (LOG_NOTICE,
     "rodsServer Release version %s - API Version %s is up",
     RODS_REL_VERSION, RODS_API_VERSION);

    /* Record port, pid, and cwd into a well-known file */
    recordServerProcess(svrComm);
    /* start the irodsReServer */
#ifndef windows_platform   /* no reServer for Windows yet */
    getReHost (&reServerHost);
    if (reServerHost != NULL && reServerHost->localFlag == LOCAL_HOST) {
        if (RODS_FORK () == 0) {  /* child */
            char *reServerOption = NULL;
            char *av[NAME_LEN];

	    close (svrComm->sock);
            memset (av, 0, sizeof (av));
            reServerOption = getenv ("reServerOption");
            setExecArg (reServerOption, av);
            rodsLog(LOG_NOTICE, "Starting irodsReServer");
            av[0] = "irodsReServer";
            execv(av[0], av);
            exit(1);
        }
    }
    getXmsgHost (&xmsgServerHost);
    if (xmsgServerHost != NULL && xmsgServerHost->localFlag == LOCAL_HOST) {
        if (RODS_FORK () == 0) {  /* child */
            char *av[NAME_LEN];

            close (svrComm->sock);
            memset (av, 0, sizeof (av));
            rodsLog(LOG_NOTICE, "Starting irodsXmsgServer");
            av[0] = "irodsXmsgServer";
	    execv(av[0], av); 
            exit(1);
        }
    }
#endif

    return status;
}

/* add incoming connection request to the bottom of the link list */

int
addConnReqToQue (rsComm_t *rsComm, int sock)
{
    agentProc_t *myConnReq;

#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST_COND
    boost::unique_lock< boost::mutex > read_req_lock( ReadReqCondMutex );
    #else
    pthread_mutex_lock (&ReadReqCondMutex);
    #endif
#endif
    myConnReq = (agentProc_t*)calloc (1, sizeof (agentProc_t));

    myConnReq->sock = sock;
    myConnReq->remoteAddr = rsComm->remoteAddr;
    queAgentProc (myConnReq, &ConnReqHead, BOTTOM_POS);

#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST_COND
    ReadReqCond.notify_all(); // NOTE:: check all vs one
    read_req_lock.unlock();
    #else
    pthread_cond_signal (&ReadReqCond);
    pthread_mutex_unlock (&ReadReqCondMutex);
    #endif
#endif

    return (0);
}

int
initConnThreadEnv ()
{
#ifndef SINGLE_SVR_THR
    #ifndef USE_BOOST_COND
    pthread_mutex_init (&ReadReqCondMutex, NULL);
    pthread_mutex_init (&ConnectedAgentMutex, NULL);
    pthread_mutex_init (&SpawnReqCondMutex, NULL);
    pthread_mutex_init (&BadReqMutex, NULL);
    pthread_cond_init (&ReadReqCond, NULL);
    pthread_cond_init (&SpawnReqCond, NULL);
    #endif
#endif
    return (0);
}

agentProc_t *
getConnReqFromQue ()
{
    agentProc_t *myConnReq = NULL;

    while (myConnReq == NULL) {
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST_COND
	boost::unique_lock<boost::mutex> read_req_lock( ReadReqCondMutex );
	#else
	pthread_mutex_lock (&ReadReqCondMutex);
	#endif
#endif
        if (ConnReqHead != NULL) {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    read_req_lock.unlock();
	    #else
	    pthread_mutex_unlock (&ReadReqCondMutex);
	    #endif
#endif
            break;
        }

#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST_COND
	ReadReqCond.wait( read_req_lock );
	#else
	pthread_cond_wait (&ReadReqCond, &ReadReqCondMutex);
	#endif
#endif
        if (ConnReqHead == NULL) {
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST_COND
	read_req_lock.unlock();
	#else
	pthread_mutex_unlock (&ReadReqCondMutex);
	#endif
#endif
            continue;
        } else {
            myConnReq = ConnReqHead;
            ConnReqHead = ConnReqHead->next;
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    read_req_lock.unlock();
	    #else
	    pthread_mutex_unlock (&ReadReqCondMutex);
	    #endif
#endif
            break;
        }
    }

    return (myConnReq);
}

int
startProcConnReqThreads ()
{
    int status = 0;
#ifndef SINGLE_SVR_THR
    int i;

    initConnThreadEnv ();
    for (i = 0; i < NUM_READ_WORKER_THR; i++) {
	#ifdef USE_BOOST
	ReadWorkerThread[i] = new boost::thread( readWorkerTask );
	#else
	status = pthread_create(&ReadWorkerThread[i], NULL, 
          (void *(*)(void *)) readWorkerTask, (void *) NULL);
	#endif
	if (status < 0) {
	    rodsLog (LOG_ERROR,
	      "pthread_create of readWorker %d failed, errno = %d",
	      i, errno);
	}
    }
#ifdef USE_BOOST
    SpawnManagerThread = new boost::thread( spawnManagerTask );
#else
    status = pthread_create(&SpawnManagerThread, NULL, 
      (void *(*)(void *)) spawnManagerTask, (void *) NULL);
    #endif
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "pthread_create of spawnManage failed, errno = %d", errno);
    }

#endif

    return (status);
}

void
readWorkerTask ()
{
    agentProc_t *myConnReq = NULL;
    startupPack_t *startupPack;
    int newSock;
    int status;
    struct timeval tv;

    tv.tv_sec = READ_STARTUP_PACK_TOUT_SEC;
    tv.tv_usec = 0;
    while (1) {
        myConnReq = getConnReqFromQue ();
        if (myConnReq == NULL) {
	    /* someone else took care of it */
            continue;
        }
	newSock = myConnReq->sock;
        status = readStartupPack (newSock, &startupPack, &tv);

        if (status < 0) {
            rodsLog (LOG_NOTICE, "readStartupPack error from %s, status = %d",
              inet_ntoa (myConnReq->remoteAddr.sin_addr), status);
            sendVersion (newSock, status, 0, NULL, 0);
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST
	    boost::unique_lock<boost::mutex> bad_req_lock( BadReqMutex );
	    #else
	    pthread_mutex_lock (&BadReqMutex);
	    #endif
#endif
	    queAgentProc (myConnReq, &BadReqHead, TOP_POS);
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST
	    bad_req_lock.unlock();
	    #else
	    pthread_mutex_unlock (&BadReqMutex);
	    #endif
#endif
            mySockClose (newSock);
	} else if (startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT) {
            sendVersion (newSock, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0);
            mySockClose (newSock);
            free (myConnReq);
	} else {
            if (startupPack->clientUser[0] == '\0') {
                status = chkAllowedUser (startupPack->clientUser,
                  startupPack->clientRodsZone);
                if (status < 0) {
                    sendVersion (newSock, status, 0, NULL, 0);
                    mySockClose (newSock);
		    free (myConnReq);
		}
            }
	    myConnReq->startupPack = *startupPack;
	    free (startupPack);
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    boost::unique_lock< boost::mutex > spwn_req_lock( SpawnReqCondMutex );
	    #else
	    pthread_mutex_lock (&SpawnReqCondMutex);
	    #endif
#endif
	    queAgentProc (myConnReq, &SpawnReqHead, BOTTOM_POS);
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    SpawnReqCond.notify_all(); // NOTE:: look into notify_one vs notify_all 
	    spwn_req_lock.unlock();
	    #else
	    pthread_cond_signal (&SpawnReqCond);
	    pthread_mutex_unlock (&SpawnReqCondMutex);
	    #endif
#endif
	}
    }
}

void
spawnManagerTask ()
{
    agentProc_t *mySpawnReq = NULL;
    int status;
    uint curTime;
    uint agentQueChkTime = 0;
    while (1) {
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST_COND
	boost::unique_lock<boost::mutex> spwn_req_lock( SpawnReqCondMutex );
	SpawnReqCond.wait( spwn_req_lock );
	#else
	pthread_mutex_lock (&SpawnReqCondMutex);
	pthread_cond_wait (&SpawnReqCond, &SpawnReqCondMutex);
	#endif
#endif
	while (SpawnReqHead != NULL) {
            mySpawnReq = SpawnReqHead;
	    SpawnReqHead = mySpawnReq->next;
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    spwn_req_lock.unlock();
	    #else
	    pthread_mutex_unlock (&SpawnReqCondMutex);
	    #endif
#endif
            status = spawnAgent (mySpawnReq, &ConnectedAgentHead);
            close (mySpawnReq->sock);

            if (status < 0) {
                rodsLog (LOG_NOTICE,
                 "spawnAgent error for puser=%s and cuser=%s from %s, stat=%d",
                  mySpawnReq->startupPack.proxyUser, 
		  mySpawnReq->startupPack.clientUser,
                  inet_ntoa (mySpawnReq->remoteAddr.sin_addr), status);
                free  (mySpawnReq);
            } else {
                rodsLog (LOG_NOTICE,
                 "Agent process %d started for puser=%s and cuser=%s from %s",
                  mySpawnReq->pid, mySpawnReq->startupPack.proxyUser,
                  mySpawnReq->startupPack.clientUser,
                  inet_ntoa (mySpawnReq->remoteAddr.sin_addr));
	    }
#ifndef SINGLE_SVR_THR
	    #ifdef USE_BOOST_COND
	    spwn_req_lock.lock();
	    #else
	    pthread_mutex_lock (&SpawnReqCondMutex);
	    #endif
#endif
        }
#ifndef SINGLE_SVR_THR
	#ifdef USE_BOOST_COND
	spwn_req_lock.unlock();
	#else
	pthread_mutex_unlock (&SpawnReqCondMutex);
	#endif
#endif
	curTime = time (0);
	if (curTime > agentQueChkTime + AGENT_QUE_CHK_INT) {
	    agentQueChkTime = curTime;
	    procBadReq ();
#if 0	/* take this out for now. Wayne saw problem with accept */
	    chkConnectedAgentProcQue ();
#endif
	}
    }
}

int
procSingleConnReq (agentProc_t *connReq)
{
    startupPack_t *startupPack;
    int newSock;
    int status;

    if (connReq == NULL) return USER__NULL_INPUT_ERR;

    newSock = connReq->sock;

    status = readStartupPack (newSock, &startupPack, NULL);

    if (status < 0) {
        rodsLog (LOG_NOTICE, "readStartupPack error from %s, status = %d",
          inet_ntoa (connReq->remoteAddr.sin_addr), status);
        sendVersion (newSock, status, 0, NULL, 0);
        mySockClose (newSock);
	return status;
    }
#ifdef SYS_TIMING
    printSysTiming ("irodsServer", "read StartupPack", 0);
#endif
    if (startupPack->connectCnt > MAX_SVR_SVR_CONNECT_CNT) {
        sendVersion (newSock, SYS_EXCEED_CONNECT_CNT, 0, NULL, 0);
        mySockClose (newSock);
        return SYS_EXCEED_CONNECT_CNT;
    }

    connReq->startupPack = *startupPack;
    free (startupPack);
    status = spawnAgent (connReq, &ConnectedAgentHead);

#ifndef windows_platform
    close (newSock);
#endif
    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "spawnAgent error for puser=%s and cuser=%s from %s, status = %d",
          connReq->startupPack.proxyUser, connReq->startupPack.clientUser,
          inet_ntoa (connReq->remoteAddr.sin_addr), status);
    } else {
        rodsLog (LOG_NOTICE,
         "Agent process %d started for puser=%s and cuser=%s from %s",
          connReq->pid, connReq->startupPack.proxyUser,
          connReq->startupPack.clientUser,
          inet_ntoa (connReq->remoteAddr.sin_addr));
    }
    return status;
}

/* procBadReq - process bad request */
int
procBadReq ()
{
    agentProc_t *tmpConnReq, *nextConnReq;
    /* just free them for now */
#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    boost::unique_lock< boost::mutex > bad_req_lock( BadReqMutex );
    #else
    pthread_mutex_lock (&BadReqMutex);
    #endif
#endif
    tmpConnReq = BadReqHead;
    while (tmpConnReq != NULL) {
	nextConnReq = tmpConnReq->next;
	free (tmpConnReq);
	tmpConnReq = nextConnReq;
    }
    BadReqHead = NULL;
#ifndef SINGLE_SVR_THR
    #ifdef USE_BOOST
    bad_req_lock.unlock();
    #else
    pthread_mutex_unlock (&BadReqMutex);
    #endif
#endif

    return 0;
}
    
