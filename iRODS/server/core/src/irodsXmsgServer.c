/*** Copyright (c), The Regents of the University of California            ***   *** For more information please refer to files in the COPYRIGHT directory ***/

/* irodsXmsgServer.c - The irods xmsg server
 */

#ifndef  USE_BOOST
#include <pthread.h>
#endif  /*n USE_BOOST */
#include "reconstants.h"
#include "irodsXmsgServer.h"
#include "xmsgLib.h"
#include "rsGlobal.h"
#if defined(USE_BOOST) || defined(RUN_SERVER_AS_ROOT)
#include "miscServerFunct.h"
#endif /* USE_BOOST || RUN_SERVER_AS_ROOT */

int loopCnt=-1;  /* make it -1 to run infinitel */


int
main(int argc, char **argv)
{
    int status;
    int c;
    int runMode = IRODS_SERVER;
    int flagval = 0;
    char *logDir = NULL;
    char *tmpStr;
    int logFd;

    ProcessType = XMSG_SERVER_PT;

#ifdef RUN_SERVER_AS_ROOT
#ifndef windows_platform
    if (initServiceUser() < 0) {
        exit (1);
    }
#endif
#endif

#ifndef _WIN32
    signal(SIGINT, signalExit);
    signal(SIGHUP, signalExit);
    signal(SIGTERM, signalExit);
    signal(SIGUSR1, signalExit);
    signal(SIGPIPE, rsPipSigalHandler);

#endif

    /* Handle option to log sql commands */
    tmpStr = getenv (SP_LOG_SQL);
    if (tmpStr != NULL) {
       rodsLogSqlReq(1);
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

    while ((c=getopt(argc, argv,"sSc:vD:")) != EOF) {
        switch (c) {
            case 's':
                runMode = SINGLE_PASS;
                break;
            case 'S':
                runMode = STANDALONE_SERVER;
                break;
            case 'v':   /* verbose */
                flagval |= v_FLAG;
                break;
            case 'c':
	      loopCnt = atoi(optarg);
                break;
            case 'D':   /* user specified a log directory */
                logDir = strdup (optarg);
                break;
            default:
                usage (argv[0]);
                exit (1);
        }
    }

    if ((logFd = logFileOpen (runMode, logDir, XMSG_SVR_LOGFILE)) < 0) {
        exit (1);
    }

    daemonize (runMode, logFd);


    status = xmsgServerMain ();
    /*
    cleanupAndExit (status); 
    */
    sleep(5);
    exit (0);
}

int usage (char *prog)
{
    fprintf(stderr, "Usage: %s [-scva] [-D logDir] \n",prog);
    return 0;
}

int
xmsgServerMain ()
{
    int status = 0;
    rsComm_t rsComm;
    rsComm_t svrComm;	/* rsComm is connection to icat, svrComm is the
			 * server's listening socket */
    fd_set sockMask;
    int numSock;
    int newSock;

    initThreadEnv ();
    initXmsgHashQue ();

    status = startXmsgThreads ();

    if (status < 0) {
        rodsLog (LOG_ERROR, 
	  "xmsgServerMain: startXmsgThreads error. status = %d", status);
        return status;
    }

    status = initRsComm (&rsComm);

    if (status < 0) {
        rodsLog (LOG_ERROR, "xmsgServerMain: initRsComm error. status = %d",
          status);
	return status;
    }

    status = initRsComm (&svrComm);

    if (status < 0) {
        rodsLog (LOG_ERROR, "xmsgServerMain: initRsComm error. status = %d",
          status);
        return status;
    }

#ifdef RULE_ENGINE_N
    status = initAgent (RULE_ENGINE_NO_CACHE, &rsComm);
#else
    status = initAgent (&rsComm);
#endif

    if (status < 0) {
        rodsLog (LOG_ERROR, "xmsgServerMain: initServer error. status = %d",
          status);
	return (status);
    }

    /* open  a socket an listen for connection */
    svrComm.sock = sockOpenForInConn (&svrComm, &svrComm.myEnv.xmsgPort, NULL,
      SOCK_STREAM);

    if (svrComm.sock < 0) {
        rodsLog (LOG_NOTICE, "serverMain: sockOpenForInConn error. status = %d",
          svrComm.sock);
        exit (1);
    }

    listen (svrComm.sock, MAX_LISTEN_QUE);

    FD_ZERO(&sockMask);

    rodsLog (LOG_NOTICE, "rodsServer version %s is up", RODS_REL_VERSION);

    while (1) {         /* infinite loop */
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

        newSock = rsAcceptConn (&svrComm);

        if (newSock < 0) {
            rodsLog (LOG_NOTICE,
              "serverMain: acceptConn () error, errno = %d", errno);
            continue;
        }


	addReqToQue (newSock);

	if (loopCnt > 0) {
	  loopCnt--;
	  if (loopCnt == 0){
	    return(0);
	  }
	}
	  

    }
    /* RAJA removed June 13, 2088 to avoid compiler warning in solaris
    return status;
    */
}

