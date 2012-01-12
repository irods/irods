/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsProcStat.c - server routine that handles the the ProcStat
 * API
 */

/* script generated code */
#include "procStat.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "miscServerFunct.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

int
rsProcStat (rsComm_t *rsComm, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut)
{
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    if (*procStatInp->rodsZone != '\0') {
        remoteFlag = getRcatHost (MASTER_RCAT, procStatInp->rodsZone, 
	  &rodsServerHost);
        if (remoteFlag < 0) {
            rodsLog (LOG_ERROR,
             "rsProcStat: getRcatHost() failed. erro=%d", remoteFlag);
            return (remoteFlag);
        }
        if (rodsServerHost->localFlag == REMOTE_HOST) {
	    status = remoteProcStat (rsComm, procStatInp, procStatOut,
	      rodsServerHost);
	} else {
	    status = _rsProcStat (rsComm, procStatInp, procStatOut);
	}
    } else {
	status = _rsProcStat (rsComm, procStatInp, procStatOut);
    }
    return status;
}

int
_rsProcStat (rsComm_t *rsComm, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut)
{
    int status;
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    rodsHostAddr_t addr;
    procStatInp_t myProcStatInp;
    char *tmpStr;

    if (getValByKey (&procStatInp->condInput, ALL_KW) != NULL) {
	status = _rsProcStatAll (rsComm, procStatInp, procStatOut); 
	return status;
    }
    if (getValByKey (&procStatInp->condInput, EXEC_LOCALLY_KW) != NULL) {
        status = localProcStat (rsComm, procStatInp, procStatOut);
        return status;
    }

    bzero (&addr, sizeof (addr));
    bzero (&myProcStatInp, sizeof (myProcStatInp));
    if (*procStatInp->addr != '\0') {	/* given input addr */
        rstrcpy (addr.hostAddr, procStatInp->addr, LONG_NAME_LEN);
        remoteFlag = resolveHost (&addr, &rodsServerHost);
    } else if ((tmpStr = getValByKey (&procStatInp->condInput, RESC_NAME_KW)) 
      != NULL) {
	rescGrpInfo_t *rescGrpInfo = NULL;
        status = _getRescInfo (rsComm, tmpStr, &rescGrpInfo);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "_rsProcStat: _getRescInfo of %s error. stat = %d",
              tmpStr, status);
            return status;
        }
        rstrcpy (procStatInp->addr, rescGrpInfo->rescInfo->rescLoc, NAME_LEN);
	rodsServerHost = (rodsServerHost_t*)rescGrpInfo->rescInfo->rodsServerHost;
	if (rodsServerHost == NULL) {
	    remoteFlag = SYS_INVALID_SERVER_HOST;
	} else {
	    remoteFlag = rodsServerHost->localFlag;
	}
    } else {
	/* do the IES server */
        remoteFlag = getRcatHost (MASTER_RCAT, NULL, &rodsServerHost);
    }
    if (remoteFlag < 0) {
        rodsLog (LOG_ERROR,
         "_rsProcStat: getRcatHost() failed. erro=%d", remoteFlag);
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
	addKeyVal (&myProcStatInp.condInput, EXEC_LOCALLY_KW, "");
	status = remoteProcStat (rsComm, &myProcStatInp, procStatOut,
          rodsServerHost);
	rmKeyVal (&myProcStatInp.condInput, EXEC_LOCALLY_KW);
    } else {
	status = localProcStat (rsComm, procStatInp, procStatOut);
    }
    return status;
}

int
_rsProcStatAll (rsComm_t *rsComm, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut)
{
    rodsServerHost_t *tmpRodsServerHost;
    procStatInp_t myProcStatInp;
    int status;
    genQueryOut_t *singleProcStatOut = NULL;
    int savedStatus = 0;

    bzero (&myProcStatInp, sizeof (myProcStatInp));
    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
	if (getHostStatusByRescInfo (tmpRodsServerHost) == 
	  INT_RESC_STATUS_UP) {		/* don't do down resc */
	    if (tmpRodsServerHost->localFlag == LOCAL_HOST) {
		setLocalSrvAddr (myProcStatInp.addr);
	        status = localProcStat (rsComm, &myProcStatInp, 
		  &singleProcStatOut);
	    } else {
		rstrcpy (myProcStatInp.addr, tmpRodsServerHost->hostName->name,
                  NAME_LEN);
                addKeyVal (&myProcStatInp.condInput, EXEC_LOCALLY_KW, "");
                status = remoteProcStat (rsComm, &myProcStatInp, 
		  &singleProcStatOut, tmpRodsServerHost);
                rmKeyVal (&myProcStatInp.condInput, EXEC_LOCALLY_KW);
	    }
	    if (status < 0) {
	        savedStatus = status;
	    }
	    if (singleProcStatOut != NULL) {
		if (*procStatOut == NULL) {
		    *procStatOut = singleProcStatOut;
		} else {
		    catGenQueryOut (*procStatOut, singleProcStatOut,
		      MAX_PROC_STAT_CNT);
		    freeGenQueryOut (&singleProcStatOut);
		}
		singleProcStatOut = NULL;
	    }
	}
	tmpRodsServerHost = tmpRodsServerHost->next;
    }
    return savedStatus;
}

int
localProcStat (rsComm_t *rsComm, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut)
{
    int numProc, status;
    procLog_t procLog;
    char childPath[MAX_NAME_LEN];
#ifndef USE_BOOST_FS
    DIR *dirPtr;
    struct dirent *myDirent;
#ifndef windows_platform
    struct stat statbuf;
#else
    struct irodsntstat statbuf;
#endif
#endif
    int count = 0;

    numProc = getNumFilesInDir (ProcLogDir) + 2; /* add 2 to give some room */

    bzero (&procLog, sizeof (procLog));
    /* init serverAddr */
    if (*procStatInp->addr != '\0') {   /* given input addr */
        rstrcpy (procLog.serverAddr, procStatInp->addr, NAME_LEN);
    } else {
	setLocalSrvAddr (procLog.serverAddr);
    }
    if (numProc <= 0) {
        /* add an empty entry with only serverAddr */
        initProcStatOut (procStatOut, 1);
	addProcToProcStatOut (&procLog, *procStatOut);
        return numProc;
    } else {
        initProcStatOut (procStatOut, numProc);
    }

    /* loop through the directory */
#ifdef USE_BOOST_FS
    path srcDirPath (ProcLogDir);
    if (!exists(srcDirPath) || !is_directory(srcDirPath)) {
#else
    dirPtr = opendir (ProcLogDir);
    if (dirPtr == NULL) {
#endif
        status = USER_INPUT_PATH_ERR - errno;
        rodsLogError (LOG_ERROR, status,
        "localProcStat: opendir local dir error for %s", ProcLogDir);
        return status;
    }
#ifdef USE_BOOST_FS
    directory_iterator end_itr; // default construction yields past-the-end
    for (directory_iterator itr(srcDirPath); itr != end_itr;++itr) {
        path p = itr->path();
	path cp = p.filename();
	if (!isdigit (*cp.c_str())) continue;   /* not a pid */
        snprintf (childPath, MAX_NAME_LEN, "%s",
          p.c_str ());
#else
    while ((myDirent = readdir (dirPtr)) != NULL) {
        if (strcmp (myDirent->d_name, ".") == 0 ||
          strcmp (myDirent->d_name, "..") == 0) {
            continue;
        }
	if (!isdigit (*myDirent->d_name)) continue;   /* not a pid */
        snprintf (childPath, MAX_NAME_LEN, "%s/%s", ProcLogDir, 
	  myDirent->d_name);
#endif
#ifdef USE_BOOST_FS
	if (!exists (p)) {
#else
#ifndef windows_platform
        status = stat (childPath, &statbuf);
#else
        status = iRODSNt_stat(childPath, &statbuf);
#endif
        if (status != 0) {
#endif  /* USE_BOOST_FS */
            rodsLogError (LOG_ERROR, status,
              "localProcStat: stat error for %s", childPath);
            continue;
        }
#ifdef USE_BOOST_FS
	if (is_regular_file(p)) {
#else
        if (statbuf.st_mode & S_IFREG) {
#endif
	    if (count >= numProc) {
                rodsLog (LOG_ERROR, 
                  "localProcStat: proc count %d exceeded", numProc);
		break;
	    }
#ifdef USE_BOOST_FS
	    procLog.pid = atoi (cp.c_str());
#else
	    procLog.pid = atoi (myDirent->d_name);
#endif
	    if (readProcLog (procLog.pid, &procLog) < 0) continue;
	    status = addProcToProcStatOut (&procLog, *procStatOut);
	    if (status < 0) continue;
            count++;
        } else {
            continue;
        }
    }
#ifndef USE_BOOST_FS
    closedir (dirPtr);
#endif
    return 0;
}

int
remoteProcStat (rsComm_t *rsComm, procStatInp_t *procStatInp,
genQueryOut_t **procStatOut, rodsServerHost_t *rodsServerHost)
{
    int status;
    procLog_t procLog;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_ERROR,
          "remoteProcStat: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if (procStatInp == NULL || procStatOut == NULL) return USER__NULL_INPUT_ERR;

    status = svrToSvrConnect (rsComm, rodsServerHost);

    if (status >= 0) {
        status = rcProcStat (rodsServerHost->conn, procStatInp, procStatOut);
    }
    if (status < 0 && *procStatOut == NULL) {
	/* add an empty entry */
        initProcStatOut (procStatOut, 1);
	bzero (&procLog, sizeof (procLog));
	rstrcpy (procLog.serverAddr, rodsServerHost->hostName->name,
          NAME_LEN);
        addProcToProcStatOut (&procLog, *procStatOut);
    }
    return status;
}

int 
initProcStatOut (genQueryOut_t **procStatOut, int numProc)
{
    genQueryOut_t *myProcStatOut;

    if (procStatOut == NULL || numProc <= 0) return USER__NULL_INPUT_ERR;

    myProcStatOut = *procStatOut = (genQueryOut_t*)malloc (sizeof (genQueryOut_t));
    bzero (myProcStatOut, sizeof (genQueryOut_t));

    myProcStatOut->continueInx = -1;

    myProcStatOut->attriCnt = 9;

    myProcStatOut->sqlResult[0].attriInx = PID_INX;
    myProcStatOut->sqlResult[0].len = NAME_LEN;
    myProcStatOut->sqlResult[0].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[0].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[1].attriInx = STARTTIME_INX;
    myProcStatOut->sqlResult[1].len = NAME_LEN;
    myProcStatOut->sqlResult[1].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[1].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[2].attriInx = CLIENT_NAME_INX;
    myProcStatOut->sqlResult[2].len = NAME_LEN;
    myProcStatOut->sqlResult[2].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[2].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[3].attriInx = CLIENT_ZONE_INX;
    myProcStatOut->sqlResult[3].len = NAME_LEN;
    myProcStatOut->sqlResult[3].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[3].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[4].attriInx = PROXY_NAME_INX;
    myProcStatOut->sqlResult[4].len = NAME_LEN;
    myProcStatOut->sqlResult[4].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[4].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[5].attriInx = PROXY_ZONE_INX;
    myProcStatOut->sqlResult[5].len = NAME_LEN;
    myProcStatOut->sqlResult[5].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[5].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[6].attriInx = REMOTE_ADDR_INX;
    myProcStatOut->sqlResult[6].len = NAME_LEN;
    myProcStatOut->sqlResult[6].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[6].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[7].attriInx = SERVER_ADDR_INX;
    myProcStatOut->sqlResult[7].len = NAME_LEN;
    myProcStatOut->sqlResult[7].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[7].value, NAME_LEN * numProc);

    myProcStatOut->sqlResult[8].attriInx = PROG_NAME_INX;
    myProcStatOut->sqlResult[8].len = NAME_LEN;
    myProcStatOut->sqlResult[8].value =
      (char*)malloc (NAME_LEN * numProc);
    bzero (myProcStatOut->sqlResult[8].value, NAME_LEN * numProc);


    return 0;    
}

int
addProcToProcStatOut (procLog_t *procLog, genQueryOut_t *procStatOut)
{
    int rowCnt;

    if (procLog == NULL || procStatOut == NULL) return USER__NULL_INPUT_ERR;
    rowCnt = procStatOut->rowCnt;

    snprintf (&procStatOut->sqlResult[0].value[NAME_LEN * rowCnt],
      NAME_LEN, "%d", procLog->pid);
    snprintf (&procStatOut->sqlResult[1].value[NAME_LEN * rowCnt],
      NAME_LEN, "%u", procLog->startTime);
    rstrcpy (&procStatOut->sqlResult[2].value[NAME_LEN * rowCnt],
      procLog->clientName, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[3].value[NAME_LEN * rowCnt],
      procLog->clientZone, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[4].value[NAME_LEN * rowCnt],
      procLog->proxyName, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[5].value[NAME_LEN * rowCnt],
      procLog->proxyZone, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[6].value[NAME_LEN * rowCnt],
      procLog->remoteAddr, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[7].value[NAME_LEN * rowCnt],
      procLog->serverAddr, NAME_LEN);
    rstrcpy (&procStatOut->sqlResult[8].value[NAME_LEN * rowCnt],
      procLog->progName, NAME_LEN);

    procStatOut->rowCnt++;

    return 0;
}

