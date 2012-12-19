/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * @file  reIn2p3SysRule.c
 *
 */

/*** Copyright (c) 2007 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */

#include "reIn2p3SysRule.h"
#include "genQuery.h"
#ifndef windows_platform
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static pthread_mutex_t my_mutex;
#endif

short threadIsAlive[MAX_NSERVERS];

int rodsMonPerfLog(char *serverName, char *resc, char *output, ruleExecInfo_t *rei) {

    char condstr[MAX_NAME_LEN], fname[MAX_NAME_LEN], msg[MAX_MESSAGE_SIZE], splc[MAX_VALUE][MAX_NAME_LEN], 
        splresc[MAX_VALUE][MAX_NAME_LEN], spldsk[MAX_VALUE][MAX_NAME_LEN], splmb[MAX_VALUE][MAX_NAME_LEN],
        monStatus[MAX_NAME_LEN], suffix[MAX_VALUE], *result;
    const char *delim1 = "#";
    const char *delim2 = ",";
    int indx, timestamp, day, rc1, rc2, rc3, rc4;
    FILE *foutput;
    time_t tps;
    generalRowInsertInp_t generalRowInsertInp;
    generalAdminInp_t generalAdminInp1, generalAdminInp2;
    genQueryInp_t genQueryInp;
    struct tm *now;
  
    genQueryOut_t *genQueryOut = NULL;
    tps = time(NULL);
    now = localtime(&tps);
  
    /* a quick test in order to see if the resource is up or down (needed to update the "status" metadata) */
    if ( strcmp(output, MON_OUTPUT_NO_ANSWER) == 0 ) {
        strncpy(monStatus, RESC_AUTO_DOWN, MAX_NAME_LEN);
    }
    else {
        strncpy(monStatus, RESC_AUTO_UP, MAX_NAME_LEN);
    }
  
    strSplit(output, delim1, splc);
    strSplit(resc, delim2, splresc);
    strSplit(splc[4], delim2, spldsk);
    strSplit(splc[7], delim2, splmb);
    indx = 0;
    while ( strcmp(splresc[indx],"") != 0 ) {
        if ( strcmp(monStatus, RESC_AUTO_DOWN) == 0 ) {
            rstrcpy(spldsk[indx], "-1", MAX_NAME_LEN);
            rstrcpy(splmb[indx], "-1", MAX_NAME_LEN);
        }
        sprintf(msg, "server=%s resource=%s cpu=%s, mem=%s, swp=%s, rql=%s, dsk=%s, nin=%s, nout=%s, dskAv(MB)=%s\n",
                serverName, splresc[indx], splc[0], splc[1], splc[2],
                splc[3], spldsk[indx], splc[5], splc[6], splmb[indx]);
        sprintf(suffix, "%d.%d.%d", now->tm_year+1900, now->tm_mon+1, now->tm_mday);
        sprintf(fname, "%s.%s", OUTPUT_MON_PERF, suffix);
        day = now->tm_mday;
        /* retrieve the system time */
        timestamp = time(&tps);
  
        /* log the result into the database as well */
        generalRowInsertInp.tableName = "serverload";
        generalRowInsertInp.arg1 = serverName;
        generalRowInsertInp.arg2 = splresc[indx];
        generalRowInsertInp.arg3 = splc[0];
        generalRowInsertInp.arg4 = splc[1];
        generalRowInsertInp.arg5 = splc[2];
        generalRowInsertInp.arg6 = splc[3];
        generalRowInsertInp.arg7 = spldsk[indx];
        generalRowInsertInp.arg8 = splc[5];
        generalRowInsertInp.arg9 = splc[6];
        /* prepare DB request to modify resource metadata: freespace and status */
        generalAdminInp1.arg0 = "modify";
        generalAdminInp1.arg1 = "resource";
        generalAdminInp1.arg2 = splresc[indx];
        generalAdminInp1.arg3 = "freespace";
        generalAdminInp1.arg4 = splmb[indx];
        generalAdminInp2.arg0 = "modify";
        generalAdminInp2.arg1 = "resource";
        generalAdminInp2.arg2 = splresc[indx];
        generalAdminInp2.arg3 = "status";
        generalAdminInp2.arg4 = monStatus;
        memset(&genQueryInp, 0, sizeof (genQueryInp));
        addInxIval(&genQueryInp.selectInp, COL_R_RESC_STATUS, 1);
        snprintf(condstr, MAX_NAME_LEN, "= '%s'", splresc[indx]);
        addInxVal(&genQueryInp.sqlCondInp, COL_R_RESC_NAME, condstr);
        genQueryInp.maxRows = MAX_SQL_ROWS;
#ifndef windows_platform
        pthread_mutex_lock(&my_mutex);
#endif
        /* append to the output log file */
        foutput = fopen(fname, "a");
        if (foutput != NULL) {
            fprintf(foutput, "time=%i %s", timestamp, msg);
            fclose(foutput); // JMC cppcheck - nullptr
        }
        
        rc1 = rsGeneralRowInsert(rei->rsComm, &generalRowInsertInp);
        rc2 = rsGeneralAdmin(rei->rsComm, &generalAdminInp1);
        rc3 = rsGenQuery(rei->rsComm, &genQueryInp, &genQueryOut);
        if ( rc3 >= 0 ) {
            result = genQueryOut->sqlResult[0].value;
            if ( strcmp(result, "\0") == 0 || ( strncmp(result,"auto-",5) == 0 && strcmp(result, monStatus) != 0 ) ) {
                rc4 = rsGeneralAdmin(rei->rsComm, &generalAdminInp2);
            }
        } else {
            rodsLog(LOG_ERROR, "msiServerMonPerf: unable to retrieve the status metadata for the resource %s", splresc[indx]);
        }
#ifndef windows_platform
        pthread_mutex_unlock(&my_mutex);
#endif
        if ( rc1 != 0 ) {
            fprintf(foutput, "time=%i : unable to insert the entries for server %s into the iCAT\n", 
                    timestamp, serverName);
        }
        if ( rc2 != 0 ) {
            rodsLog(LOG_ERROR, "msiServerMonPerf: unable to register the free space metadata for the resource %s", splresc[indx]);
        }
        if ( rc4 != 0 ) {
            rodsLog(LOG_ERROR, "msiServerMonPerf: unable to register the status metadata for the resource %s", splresc[indx]);
        }
        indx += 1;
    }
  
    clearGenQueryInp(&genQueryInp);
    freeGenQueryOut(&genQueryOut);
  
    return (0);
}

int strSplit(char *s, const char *ct, char splchain[MAX_VALUE][MAX_NAME_LEN]) {
/**********************************************
 * cut out character strings                   *
 ***********************************************/
    int i;
    char cs[MAX_NAME_LEN];
  
    if(s != NULL && ct != NULL) {
        for(i = 0; (rstrcpy(cs, strtok(s, ct), MAX_NAME_LEN)); i++) {
            /* store the adress returned by strtok */
            rstrcpy(splchain[i], cs, MAX_NAME_LEN);
            s = NULL;
        }
    }
    return 0;
}

int getListOfResc(rsComm_t *rsComm, char serverList[MAX_VALUE][MAX_NAME_LEN], int nservers, 
                  monInfo_t monList[MAX_NSERVERS], int *nlist) {
/**********************************************************
 * search in the database, the list of resources with      *
 * their associated server. If config file exist, restrict *
 * the list to serverList                                  *
 ***********************************************************/
    int i, j, k, index[MAX_NSERVERS], l, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
  
    memset(&genQueryInp, 0, sizeof(genQueryInp));
    memset(&index, -1, MAX_NSERVERS*sizeof(int));
    genQueryInp.maxRows = MAX_SQL_ROWS;

    clearGenQueryInp(&genQueryInp);
    addInxIval(&genQueryInp.selectInp, COL_R_LOC, 1);
    addInxIval(&genQueryInp.selectInp, COL_R_RESC_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_R_TYPE_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_R_VAULT_PATH, 1);

    status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
    if ( genQueryOut->rowCnt > 0 ) {
        l = 0;
        for (i=0; i<genQueryOut->attriCnt; i++) {
            for (j=0; j<genQueryOut->rowCnt; j++) {
                char *tResult;
                tResult = genQueryOut->sqlResult[i].value;
                tResult += j*genQueryOut->sqlResult[i].len;
                switch (i) {
                case 0:
                    if ( nservers >= 0 ) {
                        for (k=0; k<nservers; k++) {
                            if ( strcmp(serverList[k], tResult) == 0 ) {
                                index[j] = l;
                                l++;
                            }
                        }
                    }
                    else {
                        index[j] = l;
                        l++;
                    }
                    if ( index[j] != -1 ) {
                        rstrcpy(monList[index[j]].serverName, tResult, LONG_NAME_LEN);
                    }
                    break;
                case 1:
                    if ( index[j] != -1 ) {
                        rstrcpy(monList[index[j]].rescName, tResult, MAX_NAME_LEN);
                    }
                    break;
                case 2:
                    if ( index[j] != -1 ) {
                        rstrcpy(monList[index[j]].rescType, tResult, LONG_NAME_LEN);
                    }
                    break;
                case 3:
                    if ( index[j] != -1 ) {
                        rstrcpy(monList[index[j]].vaultPath, tResult, LONG_NAME_LEN);
                    }
                    break;
                }
            }
        }
        (*nlist) = l;
        clearGenQueryInp(&genQueryInp);
        freeGenQueryOut(&genQueryOut);
        return (0);
    }
    return (-1);
}

void *startMonScript(void *arg) {
/***********************************************************
 * launch Perl script on each server, retrieve the result  *
 * and give it to the rodsMonPerfLog function in order to  *
 * insert it into the database .                           *
 **********************************************************/
    char *output;
    msParam_t msp1, msp2, msp3, msp4, msp5, msout;
    int thrid,  status;
    int retval;
  
    thrInp_t *tinput = (thrInp_t*)arg;
#ifndef windows_platform
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
    fillStrInMsParam(&msp1, tinput->cmd);
    fillStrInMsParam(&msp2, tinput->cmdArgv);
    fillStrInMsParam(&msp3, tinput->execAddr);
    fillStrInMsParam(&msp4, tinput->hintPath);
    fillIntInMsParam(&msp5, tinput->addPathToArgv);
    thrid = tinput->threadId;
  
    threadIsAlive[thrid] = 0;
    status = msiExecCmd(&msp1, &msp2, &msp3, &msp4, &msp5, &msout, &(tinput->rei));

    if (status < 0) {
        char noanswer[MAXSTR] = MON_OUTPUT_NO_ANSWER;
        rodsLogError (LOG_ERROR, status, "Call to msiExecCmd failed in msiServerMonPerf. ");
        rodsMonPerfLog(tinput->execAddr, tinput->rescName, noanswer, &(tinput->rei));
        threadIsAlive[thrid] = 1;
        retval = -1;
#ifndef windows_platform
        pthread_exit((void *)&retval);
#endif
    }
  
    /* if (&msout != NULL) { */
    /* write into the irodsMonPerf log file */
    if ( (char *) (*((execCmdOut_t *) msout.inOutStruct)).stdoutBuf.buf != NULL) {
        output = (char *) (*((execCmdOut_t *) msout.inOutStruct)).stdoutBuf.buf;
        rodsMonPerfLog(tinput->execAddr, tinput->rescName, output, &(tinput->rei)); 
    }
    else { 
        char noanswer[MAXSTR] = MON_OUTPUT_NO_ANSWER;
        rodsLog(LOG_ERROR, "Server monitoring: no output for the server %s, status = %i \n", tinput->execAddr, status);
        rodsMonPerfLog(tinput->execAddr, tinput->rescName, noanswer, &(tinput->rei));
        threadIsAlive[thrid] = 1;
        retval = -1;
#ifndef windows_platform
        pthread_exit((void *)&retval);
#endif
    }
    /*}
      else {
      char noanswer[MAXSTR] = MON_OUTPUT_NO_ANSWER;
      rodsLog(LOG_ERROR, "Server monitoring: problem with the server %s, status = %i \n", tinput->execAddr, status);
      rodsMonPerfLog(tinput->execAddr, tinput->rescName, noanswer, 
      &(tinput->rei));
      threadIsAlive[thrid] = 1;
      retval = -1;
      #ifndef windows_platform
      pthread_exit((void *)&retval);
      #endif
      } */
  
    threadIsAlive[thrid] = 1;
  
    retval = 0;
#ifndef windows_platform
    pthread_exit((void *)&retval);
#endif
}

int checkIPaddress(char *IP, unsigned char IPcomp[IPV4]) {
/* function to check if the input string is a valid IP address */
    const char *delimIP = ".";
    char *eltstrIP, IPclone[MAX_NAME_LEN];
    int i, nelt = 0;

    strcpy(IPclone, IP);
    i = atoi(strtok(IPclone, delimIP));
    if ( i < 0 || i > 255 ) return -1;
    IPcomp[0] = i;
    while ( (eltstrIP = strtok(NULL, delimIP)) ) {
        nelt++;
        i = atoi(eltstrIP);
        if ( i < 0 || i > 255 ) return -1;
        IPcomp[nelt] = i;
    }
    if ( (nelt + 1) != IPV4 ) return -1;
    return (0);

}

int checkHostAccessControl (char *username, char *hostclient, char *groupsname)
{

    char *configDir, hostControlAccessFile[LONG_NAME_LEN];
    char grouplist[MAX_SQL_ROWS][MAXSTR];
    const char *delim = " \t\n";
    int groupok, i, indxc, iok, nelt;
    char line[MAXLEN], *eltstr, tempArr[NFIELDS][MAXLEN];
    unsigned char result, IPEntry[IPV4], subnetEntry[IPV4], visitorIP[IPV4];
    FILE *fp;

    /* try to open the HostControlAccess if it exists. */
    configDir = getConfigDir ();
    snprintf (hostControlAccessFile, LONG_NAME_LEN, "%s/%s", configDir,
              HOST_ACCESS_CONTROL_FILE);
    fp = fopen(hostControlAccessFile, "r");
    if (fp == NULL) {
        rodsLog (LOG_NOTICE,
                 "hostAuthCheck: can't open HostControlAccess file %s", hostControlAccessFile);
        return (UNIX_FILE_OPEN_ERR - errno);
    }
    /* parse the list of groups for the user from the groupsname char */
    nelt = 0;
    strncpy(grouplist[0], strtok(groupsname, delim), MAXSTR);
    while ( (eltstr = strtok(NULL, delim)) ) {
        nelt++;
        strncpy(grouplist[nelt], eltstr, MAXSTR);
    }
    /* parse HostControlAccess and check if <user,IP,group> is allowed to access this server. */
    while ( !feof(fp) ) {
        indxc = 0;
        if( fgets(line, MAXLEN, fp) ) {
            if ( line[0] != '#' && line[0] != '\n' ) {  /* Comment or empty line, ignore */
                eltstr = strtok(line, delim);
                strncpy(tempArr[indxc], eltstr, MAXSTR);
                while ( (eltstr = strtok(NULL, delim)) ) {
                    indxc++;
                    strncpy(tempArr[indxc], eltstr, MAXSTR);
                }
                if ( (indxc+1) == NFIELDS && checkIPaddress(tempArr[2], IPEntry) == 0 &&
                     checkIPaddress(tempArr[3], subnetEntry) == 0 &&
                     checkIPaddress(hostclient, visitorIP) == 0 ) {
                    /* check through if one of the group does correspond to the one allowed */
                    groupok = 1;
                    for ( i = 0; i <= nelt; i++ ) {
                        if ( strcmp(tempArr[1], grouplist[i]) == 0 ) {
                            groupok = 0;
                            break;
                        }
                    }
                    if ( strcmp(tempArr[1], "all") == 0 || groupok == 0 ) {
                        if ( strcmp(tempArr[0], "all") == 0 || strcmp(tempArr[0], username) == 0 ) {
                            iok = 1;
                            /* check if <client, group, clientIP> match this entry of the control access file
                               (iok=1). Get out immediatly from this function: client is allowed to proceed. */
                            for ( i = 0; i < IPV4; i++ ) {
                                result = ~( visitorIP[i]  ^ IPEntry[i] ) | subnetEntry[i];
                                if ( result != 255 ) {
                                    iok = 0;
                                }
                            }
                            if ( iok == 1 ) { fclose( fp ); return (0); } // JMC cppcheck - resource
                        }
                    }
                }
            }
        }
    }
    fclose( fp ); // JMC cppcheck - resource
    return (-1);
}

/**
 * \fn msiCheckHostAccessControl (ruleExecInfo_t *rei)
 *
 * \brief  This microservice sets the access control policy. It checks the
 *  access control by host and user based on the the policy given in the
 *  HostAccessControl file.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author Jean-Yves Nief
 * \date 2007-09 
 * 
 * \note  This microservice controls access to the iRODS service
 *  based on the information in the host based access configuration file:
 *  HOST_ACCESS_CONTROL_FILE
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre N/A
 * \post N/A
 * \sa N/A
 **/
int msiCheckHostAccessControl (ruleExecInfo_t *rei) {
    char group[MAX_NAME_LEN], *hostclient, *result, *username;
    char condstr[MAX_NAME_LEN];
    int i, rc, status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    rsComm_t *rsComm;
  
    RE_TEST_MACRO ("    Calling msiCheckHostAccessControl")
        /* the above line is needed for loop back testing using irule -i option */
    
        group[0] = '\0';
    rsComm = rei->rsComm;
  
    /* retrieve user name */
    username = rsComm->clientUser.userName;
    /* retrieve client IP address */
    hostclient = inet_ntoa(rsComm->remoteAddr.sin_addr);
    /* retrieve groups to which the user belong */
    memset(&genQueryInp, 0, sizeof (genQueryInp));
    snprintf(condstr, MAX_NAME_LEN, "= '%s'", username);
    addInxVal(&genQueryInp.sqlCondInp, COL_USER_NAME, condstr);
    addInxIval(&genQueryInp.selectInp, COL_USER_GROUP_NAME, 1);
    genQueryInp.maxRows = MAX_SQL_ROWS;
    status =  rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
    if ( status >= 0 ) {
        for (i=0; i<genQueryOut->rowCnt; i++) {
            result = genQueryOut->sqlResult[0].value;
            result += i*genQueryOut->sqlResult[0].len;
            strcat(group, result);
            strcat(group, " ");
        }
    } else {
        rstrcpy(group, "all", MAX_NAME_LEN);
    }
    clearGenQueryInp(&genQueryInp);
    freeGenQueryOut(&genQueryOut);

    rc = checkHostAccessControl(username, hostclient, group);
    if ( rc < 0 ) {
        rodsLog (LOG_NOTICE, "Access to user %s from host %s has been refused.\n", username, hostclient);
    }
  
    return (rei->status);

}


/**
 * \fn msiServerMonPerf (msParam_t *verb, msParam_t *ptime, ruleExecInfo_t *rei)
 *
 * \brief  This microservice monitors the servers' activity and performance.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author Jean-Yves Nief 
 * \date 2009-06
 * 
 * \note  This microservice monitors the servers' activity and performance
 *    for CPU, network, memory and more.  It retrieves the list of servers
 *    to monitor from the MON_CFG_FILE if it exists, or the iCAT if the
 *    configuration file does not exist.
 *
 * \note The MON_PERF_SCRIPT is executed on each host. The result is put
 *    in the OUTPUT_MON_PERF file and will also be put in the iCAT in the
 *    near future.
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/Resource_Monitoring_System
 *
 * \param[in] verb - a msParam of type STR_MS_T defining verbose mode:
 *    \li "default" - not verbose
 *    \li "verbose" - verbose mode
 * \param[in] ptime - a msParam of type STR_MS_T defining probe time
 *    in seconds. "default" is equal to 10 seconds.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified resource status flag, resource free space available, 
 table R_SERVER_LOAD
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre N/A
 * \post N/A
 * \sa N/A
 **/
int msiServerMonPerf (msParam_t *verb, msParam_t *ptime, ruleExecInfo_t *rei) {
    char buffer[MAX_NAME_LEN], line[MAX_VALUE], splchain[MAX_VALUE][MAX_NAME_LEN], *verbosity;
    char serverList[MAX_VALUE][MAX_NAME_LEN];
    char cmd[MAX_NAME_LEN]; /* cmd => name of the Perl script */
    char probtime[LEN_SECONDS], measTime[LEN_SECONDS];
    FILE *filein; /* file pointers */
    const char *delim = " \n";
    char valinit[MAX_NAME_LEN] = "";
    char val[MAX_NAME_LEN] = ""; /* val => arguments for the script */
    int check, i, indx, j, looptime, maxtime, nresc, nservers, rc, thrCount, threadsNotfinished;
    const char *probtimeDef = "10"; /* default value used by the monitoring script for the amount
                                       of time for this measurement (in s) */
    rsComm_t *rsComm;
    monInfo_t rescList[MAX_NSERVERS];
    thrInp_t *thrInput;
    int addPathToArgv = 0;
    char *hintPath = "";
  
    RE_TEST_MACRO ("    Calling msiServerMonPerf")
    
        /* the above line is needed for loop back testing using irule -i option */
    
        rsComm = rei->rsComm;
  
    if (verb->inOutStruct != NULL) {
        verbosity = (char *) verb->inOutStruct;
        if (strcmp(verbosity, "verbose") == 0 ) {
            strcat(valinit, "-v ");
        }
    }
  
    strcat(valinit, " -t ");
  
    strncpy(probtime, (char *) ptime->inOutStruct,  LEN_SECONDS);
    if( atoi(probtime) > 0) {
        strcat(valinit, probtime);
        strncpy(measTime, probtime, LEN_SECONDS);
    }
    else {
        strcat(valinit, probtimeDef);
        strncpy(measTime, probtimeDef, LEN_SECONDS);
    }
  
    rstrcpy(val, "", MAX_NAME_LEN);
  
    /* read the config file or the iCAT to know the servers list to monitor */
    nresc = 0;
  
    nservers = -1;  /* nservers = -1, no config file available, consider all ressources for the monitoring */
    /* nservers >= 0, config file available, consider all resources hosted on the list of servers */
    if((filein = fopen(MON_CFG_FILE, "r")) != NULL) {
        i = 0;
        while(fgets(line, sizeof line, filein) != NULL) { /* for each line of the file */
            /* if begin of line = # => ignore */
            if(line[0] != '#') {
                rstrcpy(buffer, strdup(line), MAX_NAME_LEN);
                strSplit(buffer, delim, splchain);
                rstrcpy(serverList[i], splchain[0], MAX_NAME_LEN);
                i++;
            } 
        }
        /* number of servers... useful for the threads */
        nservers = i;
        /* close the configuration file */
        fclose(filein);
    }
    getListOfResc(rsComm, serverList, nservers, rescList, &nresc);
  
    strcpy(cmd, MON_PERF_SCRIPT);
#ifndef windows_platform
    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * nresc);
    pthread_mutex_init(&my_mutex, NULL);
#endif
    thrInput = (thrInp_t*)malloc(sizeof(thrInp_t) * nresc);
    thrCount = 0;
  
    for (i = 0; i < nresc; i++) {
        /* for each server, build the proxy command to be executed.
           it will be put in a thrInp structure to be given to the thread.
           then start one thread for each server to be monitored */
        check = 0;
        for (j = 0; j < thrCount; j++) {
            if ( strcmp(thrInput[j].execAddr, rescList[i].serverName) == 0 ) {
                indx = j;
                check = 1;
            }
        }
        if ( check == 0 ) {
            strcpy(thrInput[thrCount].cmdArgv, valinit);
            strcat(thrInput[thrCount].cmdArgv, " -fs ");
            if ( strcmp(rescList[thrCount].rescType, "unix file system") == 0 ) {
                strcat(thrInput[thrCount].cmdArgv, rescList[i].vaultPath);
            } else {
                strcat(thrInput[thrCount].cmdArgv, "none");
            }
            rstrcpy(thrInput[thrCount].cmd, cmd, LONG_NAME_LEN);
            rstrcpy(thrInput[thrCount].execAddr, rescList[i].serverName, LONG_NAME_LEN);
            rstrcpy(thrInput[thrCount].hintPath, hintPath, MAX_NAME_LEN);
            thrInput[thrCount].addPathToArgv = addPathToArgv;
            thrInput[thrCount].threadId = thrCount;
            rstrcpy(thrInput[thrCount].rescName, rescList[i].rescName, MAX_NAME_LEN);
            memcpy(&(thrInput[thrCount].rei), rei, sizeof(ruleExecInfo_t));
            thrCount += 1;
        }
        else {
            rstrcat(thrInput[indx].rescName, ",", MAX_NAME_LEN);
            rstrcat(thrInput[indx].rescName, rescList[i].rescName, MAX_NAME_LEN);
            if ( strcmp(rescList[i].rescType, "unix file system") == 0 ) {
                strcat(thrInput[indx].cmdArgv, ",");
                strcat(thrInput[indx].cmdArgv, rescList[i].vaultPath);
            } else {
                strcat(thrInput[indx].cmdArgv, ",none");
            }
        }
        rstrcpy(val, "", MAX_NAME_LEN);
    }
  
    for ( i = 0; i < thrCount; i++ ) {
#ifndef windows_platform
        if ( pthread_create(&threads[i], NULL, *startMonScript, (void *) &thrInput[i]) < 0) {
            rodsLog(LOG_ERROR, "msiServerMonPerf: pthread_create error\n");
            exit(1);
        }
#endif
    }

    maxtime = atoi(measTime) + TIMEOUT;
    looptime = 0;
    while (1) {
        sleep(1);
        looptime += 1;
        if ( looptime >= maxtime ) {
            for (i = 0; i < thrCount; i++) {
                if ( !threadIsAlive[i] ) {
#ifndef windows_platform
                    rc = pthread_cancel(threads[i]);
#endif
                    if ( rc == 0 ) {
                        char noanswer[MAXSTR] = MON_OUTPUT_NO_ANSWER;
                        threadIsAlive[i] = 1;
                        rodsMonPerfLog(thrInput[i].execAddr,
                                       thrInput[i].rescName, 
                                       noanswer, 
                                       &(thrInput[i].rei));
                    }
                }
            }
        }
        threadsNotfinished = 1;
        for (i = 0; i < thrCount; i++) {
            if ( threadIsAlive[i] == 0 ) {
                threadsNotfinished = 0;
            }
        }
        if ( threadsNotfinished ) {
            break;
        }
    }
  
#ifndef windows_platform
    free(threads);
#endif
    free(thrInput);
  
    return (rei->status);
  
}


/**
 * \fn msiFlushMonStat (msParam_t *inpParam1, msParam_t *inpParam2, ruleExecInfo_t *rei)
 *
 * \brief  This microservice flushes the servers' monitoring statistics.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author Jean-Yves Nief
 * \date 2009-06
 * 
 * \note  This microservice removes the servers' metrics older than the
 *    number of hours in "timespan".  
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/Resource_Monitoring_System
 *
 * \param[in] inpParam1 - Required - a msParam of type STR_MS_T defining the timespan in hours.
 *    "default" is equal to 24 hours.
 * \param[in] inpParam2 - Required - a msParam of type STR_MS_T defining the tablename to be
 *    flushed.  Currently must be either "serverload" or "serverloaddigest".
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence table R_SERVER_LOAD content
 * \iCatAttrModified table R_SERVER_LOAD content
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre N/A
 * \post N/A
 * \sa  N/A
 **/
int msiFlushMonStat (msParam_t *inpParam1, msParam_t *inpParam2, ruleExecInfo_t *rei) {
    int elapseTime, defaultTimespan, rc;
    char secAgo[MAXLEN], *tablename, *timespan;
    generalRowPurgeInp_t generalRowPurgeInp;
    rsComm_t *rsComm;
  
    RE_TEST_MACRO ("    Calling msiFlushMonStat");
  
    defaultTimespan = 24;  /* in hours */
  
    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR, "msiFlushMonStat: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
  
    rsComm = rei->rsComm;
        
    if ( inpParam1 == NULL ) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiFlushMonStat: input Param1 is NULL");
        return (rei->status);
    }
  
    if ( strcmp (inpParam1->type, STR_MS_T) == 0 ) {
        timespan = (char *) inpParam1->inOutStruct;
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiFlushMonStat: Unsupported input Param1 type %s",
                            inpParam1->type);
        return (rei->status);
    }
  
    if ( inpParam2 == NULL ) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiFlushMonStat: input Param2 is NULL");
        return (rei->status);
    }
  
    if ( strcmp (inpParam2->type, STR_MS_T) == 0 ) {
        tablename = (char *) inpParam2->inOutStruct;
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiFlushMonStat: Unsupported input Param2 type %s",
                            inpParam2->type);
        return (rei->status);
    }

    if ( atoi(timespan) > 0 ) {
        elapseTime = atoi(timespan) * 3600;
    }
    else {
        elapseTime = defaultTimespan * 3600; /* default timespan in seconds */
    }
  
    if ( strcmp(tablename, "serverload") != 0 && 
         strcmp(tablename, "serverloaddigest") != 0 ) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiFlushMonStat: table %s does not exist", tablename);
        return (rei->status);
    }
  
    generalRowPurgeInp.tableName = tablename;
    snprintf (secAgo, MAXLEN, "%i", elapseTime);
    generalRowPurgeInp.secondsAgo = secAgo;
    rc = rsGeneralRowPurge(rsComm, &generalRowPurgeInp);
  
    if ( rc != 0 && rc != CAT_SUCCESS_BUT_WITH_NO_INFO ) {
        rodsLog (LOG_ERROR, "msiFlushMonStat failed, error %i", rc);
    }
  
    return (rei->status);
}


/**
 * \fn msiDigestMonStat(msParam_t *cpu_wght, msParam_t *mem_wght, msParam_t *swap_wght,
 *       msParam_t *runq_wght, msParam_t *disk_wght, msParam_t *netin_wght,
 *       msParam_t *netout_wght, ruleExecInfo_t *rei)
 *
 * \brief  This microservice calculates and stores a load factor for each connected
 *    resource based on the weighting values passed in as parameters.
 * 
 * \module core
 * 
 * \since pre-2.1
 * 
 * \author Jean-Yves Nief
 * \date 2009-06 
 * 
 * \note  The following values are loaded from R_LOAD_SERVER:
 *    \li cpu_used
 *    \li mem_used
 *    \li swap_used
 *    \li runq_load
 *    \li disk_space
 *    \li net_input
 *    \li net_output
 *
 * \note  The stored load factor is calculated as such:
 *    \li load_factor = cpu_wght*cpu_used + mem_wght*mem_used + swap_wght*swap_used +
 *        runq_wght*runq_load + disk_wght*disk_space + netin_wght*net_input +
 *        netout_wght*net_output
 *
 * \usage See clients/icommands/test/rules3.0/ and https://www.irods.org/index.php/Resource_Monitoring_System
 *
 * \param[in] cpu_wght - Required - a msParam of type STR_MS_T defining relative CPU weighting.
 * \param[in] mem_wght - Required - a msParam of type STR_MS_T defining relative memory weighting
 * \param[in] swap_wght - Required - a msParam of type STR_MS_T defining relative swap weighting
 * \param[in] runq_wght - Required - a msParam of type STR_MS_T defining relative run queue weighting
 * \param[in] disk_wght - Required - a msParam of type STR_MS_T defining relative disk space weighting
 * \param[in] netin_wght - Required - a msParam of type STR_MS_T defining relative inbound network weighting
 * \param[in] netout_wght - Required - a msParam of type STR_MS_T defining relative outbound network weighting
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence R_SERVER_LOAD table content
 * \iCatAttrModified R_SERVER_LOAD_DIGEST table content
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre N/A
 * \post N/A
 * \sa  N/A
 **/
int msiDigestMonStat(msParam_t *cpu_wght, msParam_t *mem_wght, msParam_t *swap_wght, msParam_t *runq_wght,
                     msParam_t *disk_wght, msParam_t *netin_wght, msParam_t *netout_wght, 
                     ruleExecInfo_t *rei) {
    char rescList[MAX_NSERVERS][MAX_NAME_LEN], *tResult,
        timeList[MAX_NSERVERS][MAX_NAME_LEN];
    char condStr1[MAX_NAME_LEN], condStr2[MAX_NAME_LEN], loadStr[MAX_NAME_LEN];
    int i, j, loadFactor, nresc, rc, status, totalWeight, weight[NRESULT];
    rsComm_t *rsComm;
    generalRowInsertInp_t generalRowInsertInp;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
  
    RE_TEST_MACRO ("    Calling msiDigestMonStat");
  
    if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
                 "msiDigestMonStat: input rei or rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
  
    rsComm = rei->rsComm;
  
    if ( cpu_wght == NULL || mem_wght == NULL || swap_wght == NULL || runq_wght == NULL
         || disk_wght == NULL || netin_wght == NULL || netout_wght == NULL ) {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: at least one of the input param is NULL");
        return (rei->status);
    }
  
    if ( strcmp (cpu_wght->type, STR_MS_T) == 0 ) {
        weight[0] = atoi((const char*)cpu_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input cpu_wght type %s",
                            cpu_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (mem_wght->type, STR_MS_T) == 0 ) {
        weight[1] = atoi((const char*)mem_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input mem_wght type %s",
                            mem_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (swap_wght->type, STR_MS_T) == 0 ) {
        weight[2] = atoi((const char*)swap_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input swap_wght type %s",
                            swap_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (runq_wght->type, STR_MS_T) == 0 ) {
        weight[3] = atoi((const char*)runq_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input runq_wght type %s",
                            runq_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (disk_wght->type, STR_MS_T) == 0 ) {
        weight[4] = atoi((const char*)disk_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input disk_wght type %s",
                            disk_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (netin_wght->type, STR_MS_T) == 0 ) {
        weight[5] = atoi((const char*)netin_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input netin_wght type %s",
                            netin_wght->type);
        return (rei->status);
    }
  
    if ( strcmp (netout_wght->type, STR_MS_T) == 0 ) {
        weight[6] = atoi((const char*)netout_wght->inOutStruct);
    } else {
        rodsLogAndErrorMsg (LOG_ERROR, &rsComm->rError, rei->status,
                            "msiDigestMonStat: Unsupported input netout_wght type %s",
                            netout_wght->type);
        return (rei->status);
    }
  
    totalWeight = 0;
    for (i=0; i<NRESULT; i++){
        totalWeight += weight[i];
    }
  
    memset(&genQueryInp, 0, sizeof (genQueryInp));
    addInxIval(&genQueryInp.selectInp, COL_SL_RESC_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_CREATE_TIME, SELECT_MAX);
    genQueryInp.maxRows = MAX_SQL_ROWS;
    status =  rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
    if( NULL == genQueryOut ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "msiDigestMonStat :: &genQueryOut is NULL" );
        return rei->status; 
    }
    if ( status == 0 ) {
        nresc = genQueryOut->rowCnt;
        for (i=0; i<genQueryOut->attriCnt; i++) {
            for (j=0; j<nresc; j++) {
                tResult = genQueryOut->sqlResult[i].value;
                tResult += j*genQueryOut->sqlResult[i].len;
                if ( i == 0 ) {
                    rstrcpy(rescList[j], tResult, genQueryOut->sqlResult[i].len);
                }
                if ( i == 1 ) {
                    rstrcpy(timeList[j], tResult, genQueryOut->sqlResult[i].len);
                }
            }
        }
    }
    else {
        rodsLog(LOG_ERROR, "msiDigestMonStat: Unable to retrieve information \
                        from R_SERVER_LOAD");
        return(rei->status);
    }
  
    memset(&genQueryInp, 0, sizeof (genQueryInp));
    addInxIval(&genQueryInp.selectInp, COL_SL_CPU_USED, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_MEM_USED, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_SWAP_USED, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_RUNQ_LOAD, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_DISK_SPACE, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_NET_INPUT, 1);
    addInxIval(&genQueryInp.selectInp, COL_SL_NET_OUTPUT, 1);
    genQueryInp.maxRows = 1;
    generalRowInsertInp.tableName = "serverloaddigest";
    for (i=0; i<nresc; i++) {
        memset(&genQueryInp.sqlCondInp, 0, sizeof (genQueryInp.sqlCondInp));
        snprintf(condStr1, MAX_NAME_LEN, "= '%s'", rescList[i]);
        addInxVal(&genQueryInp.sqlCondInp, COL_SL_RESC_NAME, condStr1);
        snprintf(condStr2, MAX_NAME_LEN, "= '%s'", timeList[i]);
        addInxVal(&genQueryInp.sqlCondInp, COL_SL_CREATE_TIME, condStr2);
        status =  rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
        if ( status == 0 ) {
            loadFactor = 0;
            for (j=0; j<genQueryOut->attriCnt; j++) {
                tResult = genQueryOut->sqlResult[j].value;
                loadFactor += atoi(tResult) * weight[j];
            }
            loadFactor = loadFactor / totalWeight;
            generalRowInsertInp.arg1 = rescList[i];
            snprintf(loadStr, MAX_NAME_LEN, "%i", loadFactor);
            generalRowInsertInp.arg2 = loadStr;
            rc = rsGeneralRowInsert(rsComm, &generalRowInsertInp);
            if ( rc != 0 ) {
                rodsLog(LOG_ERROR, "msiDigestMonStat: Unable to ingest\
        information into from R_SERVER_LOAD_DIGEST table");
            }
        }
    }
  
    clearGenQueryInp(&genQueryInp);
    freeGenQueryOut(&genQueryOut);
  
    return(rei->status);
}
