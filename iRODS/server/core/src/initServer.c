/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.c - Server initialization routines
 */

#ifdef USE_BOOST
#else
#ifndef windows_platform
#include <pthread.h>
#endif
#endif

#include "initServer.h"
#include "resource.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "genQuery.h"
#include "rsIcatOpr.h"
#include "miscServerFunct.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#ifdef HPSS
#include "hpssFileDriver.h"
#endif

static time_t LastBrokenPipeTime = 0;
static int BrokenPipeCnt = 0;

int
resolveHost (rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost)
{
    int status;
    rodsServerHost_t *tmpRodsServerHost;
    char *myHostAddr;
    char *myZoneName;

    /* check if host exist */

    myHostAddr = addr->hostAddr;

    if (strlen (myHostAddr) == 0) {
	*rodsServerHost = ServerHostHead;
        return LOCAL_HOST;
    }
    if (strlen (addr->zoneName) == 0) {
	myZoneName = ZoneInfoHead->zoneName;
    } else {
	myZoneName = addr->zoneName;
    }

    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
	hostName_t *tmpName;
	zoneInfo_t *serverZoneInfo = (zoneInfo_t *) tmpRodsServerHost->zoneInfo;
	if (strcmp (myZoneName, serverZoneInfo->zoneName) == 0) {
	    tmpName = tmpRodsServerHost->hostName;
	    while (tmpName != NULL) {
	        if (strcasecmp (tmpName->name, myHostAddr) == 0) { 
		    *rodsServerHost = tmpRodsServerHost;
		    return (tmpRodsServerHost->localFlag);
	        }
	        tmpName = tmpName->next;
	    }
	}
	tmpRodsServerHost = tmpRodsServerHost->next;
    }

    /* no match */ 
 
    tmpRodsServerHost = mkServerHost (myHostAddr, myZoneName);

    if (tmpRodsServerHost == NULL) {
	rodsLog (LOG_ERROR,
          "resolveHost: mkServerHost error");
        return (SYS_INVALID_SERVER_HOST);
    }

    /* assume it is remote */
    if (tmpRodsServerHost->localFlag == UNKNOWN_HOST_LOC)
        tmpRodsServerHost->localFlag = REMOTE_HOST;

    status = queRodsServerHost (&ServerHostHead, tmpRodsServerHost);
    *rodsServerHost = tmpRodsServerHost;

    return (tmpRodsServerHost->localFlag);
}

int 
resolveHostByDataObjInfo (dataObjInfo_t *dataObjInfo,
rodsServerHost_t **rodsServerHost)
{
    rodsHostAddr_t addr;
    int remoteFlag;

    if (dataObjInfo == NULL || dataObjInfo->rescInfo == NULL ||
      dataObjInfo->rescInfo->rescLoc == NULL) {
        rodsLog (LOG_NOTICE,
          "resolveHostByDataObjInfo: NULL input");
	return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    memset (&addr, 0, sizeof (addr));
    rstrcpy (addr.hostAddr, dataObjInfo->rescInfo->rescLoc, NAME_LEN);

    remoteFlag = resolveHost (&addr, rodsServerHost);

    return (remoteFlag);
}

int
resolveHostByRescInfo (rescInfo_t *rescInfo, rodsServerHost_t **rodsServerHost)
{
    rodsHostAddr_t addr;
    int remoteFlag;

    if (rescInfo == NULL || rescInfo->rescLoc == NULL) {
        rodsLog (LOG_NOTICE,
          "resolveHostByRescInfo: NULL input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    memset (&addr, 0, sizeof (addr));
    rstrcpy (addr.hostAddr, rescInfo->rescLoc, NAME_LEN);

    remoteFlag = resolveHost (&addr, rodsServerHost);

    return (remoteFlag);
}

rodsServerHost_t *
mkServerHost (char *myHostAddr, char *zoneName)
{
    rodsServerHost_t *tmpRodsServerHost;
    int status;

    tmpRodsServerHost = (rodsServerHost_t*)malloc (sizeof (rodsServerHost_t));
    memset (tmpRodsServerHost, 0, sizeof (rodsServerHost_t));

#if 0
    if (portNum > 0) {
        tmpRodsServerHost->portNum = portNum;
    } else {
        tmpRodsServerHost->portNum = ServerHostHead->portNum;
    }
#endif
    /* XXXXX need to lookup the zone table when availiable */
    status = queHostName (tmpRodsServerHost, myHostAddr, 0);
    if (status < 0) {
	free (tmpRodsServerHost);
        return (NULL);
    }

    tmpRodsServerHost->localFlag = UNKNOWN_HOST_LOC;

    status = queAddr (tmpRodsServerHost, myHostAddr);

    status = matchHostConfig (tmpRodsServerHost);

    status = getZoneInfo (zoneName, 
      (zoneInfo_t **) &tmpRodsServerHost->zoneInfo);

    if (status < 0) {
        free (tmpRodsServerHost);
        return (NULL);
    } else {
        return (tmpRodsServerHost);
    }
}

int
initServerInfo (rsComm_t *rsComm)
{
    int status;

    /* que the local zone */

    queZone (rsComm->myEnv.rodsZone, rsComm->myEnv.rodsPort, NULL, NULL);

    status = initHostConfigByFile (rsComm);
	if (status < 0) {
        rodsLog (LOG_NOTICE,
          "initServerInfo: initHostConfigByFile error, status = %d",
          status);
        return (status);
    }

    status = initLocalServerHost (rsComm);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "initServerInfo: initLocalServerHost error, status = %d",
          status);
        return (status);
    }
    status = initRcatServerHostByFile (rsComm);
    if (status < 0) {
        rodsLog (LOG_SYS_FATAL,
          "initServerInfo: initRcatServerHostByFile error, status = %d",
          status);
        return (status);
    }

#ifdef RODS_CAT
    status = connectRcat (rsComm);
    if (status < 0) {
        return (status);
    }
#endif
    status = initZone (rsComm);
    if (status < 0) {
        rodsLog (LOG_SYS_FATAL,
          "initServerInfo: initZone error, status = %d",
          status);
        return (status);
    }

    status = initResc (rsComm);
    if (status < 0) {
	if (status == CAT_NO_ROWS_FOUND) {
	    rodsLog (LOG_NOTICE, 
	     "initServerInfo: No resource is configured in ICAT");
	    status = 0;
	} else { 
            rodsLog (LOG_SYS_FATAL,
              "initServerInfo: initResc error, status = %d",
              status);
            return (status);
	}
    }

    return (status);
}

int
initLocalServerHost (rsComm_t *rsComm)
{
    int status;
    char myHostName[MAX_NAME_LEN];

    LocalServerHost = ServerHostHead = (rodsServerHost_t*)malloc (sizeof (rodsServerHost_t));
    memset (ServerHostHead, 0, sizeof (rodsServerHost_t)); 

    LocalServerHost->localFlag = LOCAL_HOST;
    LocalServerHost->zoneInfo = ZoneInfoHead;

    status = matchHostConfig (LocalServerHost);

    queHostName (ServerHostHead, "localhost", 0); 
    status = gethostname (myHostName, MAX_NAME_LEN);
    if (status < 0) {
	status = SYS_GET_HOSTNAME_ERR - errno;
        rodsLog (LOG_NOTICE, 
	  "initLocalServerHost: gethostname error, status = %d", 
	  status);
	return (status);
    }
    status = queHostName (ServerHostHead, myHostName, 0);
    if (status < 0) {
	return (status);
    }

    status = queAddr (ServerHostHead, myHostName);
    if (status < 0) {
	/* some configuration may not be able to resolve myHostName. So don't
	 * exit. Just print out warning */ 
        rodsLog (LOG_NOTICE,
          "initLocalServerHost: queAddr error, status = %d",
          status);
	status = 0;
    }

#if 0
    if (myEnv != NULL) {
        /* ServerHostHead->portNum = myEnv->rodsPort; */
	ZoneInfoHead->portNum = myEnv->rodsPort;
    }
#endif

    if (ProcessType == SERVER_PT) {
	printServerHost (LocalServerHost);
    }

    return (status);
}

int
printServerHost (rodsServerHost_t *myServerHost)
{
    hostName_t *tmpHostName;

    if (myServerHost->localFlag == LOCAL_HOST) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE,"    LocalHostName: ");
#else /* IRODS_SYSLOG */
        fprintf (stderr, "    LocalHostName: ");
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE,"    LocalHostName: ");
#endif
    } else {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE,"    RemoteHostName: ");
#else /* IRODS_SYSLOG */
        fprintf (stderr, "    RemoteHostName: ");
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE,"    RemoteHostName: ");
#endif
    }

    tmpHostName = myServerHost->hostName;

    while (tmpHostName != NULL) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE," %s,", tmpHostName->name);
#else /* IRODS_SYSLOG */
        fprintf (stderr, " %s,", tmpHostName->name);
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE," %s,", tmpHostName->name);
#endif
        tmpHostName = tmpHostName->next;
    }

#ifndef windows_platform
#ifdef IRODS_SYSLOG
	rodsLog (LOG_NOTICE," Port Num: %d.\n\n", ((zoneInfo_t *)myServerHost->zoneInfo)->portNum);
#else /* IRODS_SYSLOG */
    fprintf (stderr, " Port Num: %d.\n\n", 
      ((zoneInfo_t *)myServerHost->zoneInfo)->portNum);
#endif /* IRODS_SYSLOG */
#else
	rodsLog (LOG_NOTICE," Port Num: %d.\n\n", ((zoneInfo_t *)myServerHost->zoneInfo)->portNum);
#endif

    return (0);
}

int
printZoneInfo ()
{
    zoneInfo_t *tmpZoneInfo;
    rodsServerHost_t *tmpRodsServerHost;

    tmpZoneInfo = ZoneInfoHead;
#ifndef windows_platform
#ifdef IRODS_SYSLOG
	rodsLog (LOG_NOTICE,"Zone Info:\n");
#else /* IRODS_SYSLOG */
    fprintf (stderr, "Zone Info:\n");
#endif /* IRODS_SYSLOG */
#else
	rodsLog (LOG_NOTICE,"Zone Info:\n");
#endif
    while (tmpZoneInfo != NULL) {
	/* print the master */
        tmpRodsServerHost = (rodsServerHost_t *) tmpZoneInfo->masterServerHost;
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE,"    ZoneName: %s   ", tmpZoneInfo->zoneName);
#else /* IRODS_SYSLOG */
		fprintf (stderr, "    ZoneName: %s   ", tmpZoneInfo->zoneName);
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE,"    ZoneName: %s   ", tmpZoneInfo->zoneName);
#endif
	if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE,"Type: LOCAL_ICAT   "); 
#else /* IRODS_SYSLOG */
	    fprintf (stderr, "Type: LOCAL_ICAT   "); 
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE,"Type: LOCAL_ICAT   "); 
#endif
	} else {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE,"Type: REMOTE_ICAT   ");
#else /* IRODS_SYSLOG */
	    fprintf (stderr, "Type: REMOTE_ICAT   "); 
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE,"Type: REMOTE_ICAT   ");
#endif
	}

#ifndef windows_platform
#ifdef IRODS_SYSLOG
	rodsLog (LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n", 
	  tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#else /* IRODS_SYSLOG */
        fprintf (stderr, " HostAddr: %s   PortNum: %d\n\n", 
	  tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#endif /* IRODS_SYSLOG */
#else
	rodsLog (LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n", 
	  tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#endif

	/* print the slave */
        tmpRodsServerHost = (rodsServerHost_t *) tmpZoneInfo->slaveServerHost;
	if (tmpRodsServerHost != NULL) { 
#ifndef windows_platform
#ifdef IRODS_SYSLOG
		rodsLog (LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName);
		rodsLog (LOG_NOTICE, "Type: LOCAL_SLAVE_ICAT   ");
		rodsLog (LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
              tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#else /* IRODS_SYSLOG */
            fprintf (stderr, "    ZoneName: %s   ", tmpZoneInfo->zoneName);
            fprintf (stderr, "Type: LOCAL_SLAVE_ICAT   ");
            fprintf (stderr, " HostAddr: %s   PortNum: %d\nn",
              tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#endif /* IRODS_SYSLOG */
#else
		rodsLog (LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName);
		rodsLog (LOG_NOTICE, "Type: LOCAL_SLAVE_ICAT   ");
		rodsLog (LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
              tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum);
#endif
	}

	tmpZoneInfo = tmpZoneInfo->next;
    }
    /* print the reHost */
    if (getReHost (&tmpRodsServerHost) >= 0) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"reHost:   %s", tmpRodsServerHost->hostName->name);
#else /* IRODS_SYSLOG */
        fprintf (stderr, "reHost:   %s\n\n", tmpRodsServerHost->hostName->name);
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_NOTICE,"reHost:   %s", tmpRodsServerHost->hostName->name);
#endif
    } else {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_ERROR,"reHost error");
#else /* IRODS_SYSLOG */
        fprintf (stderr,"reHost error");
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_ERROR,"reHost error");
#endif
    }

    if (getXmsgHost (&tmpRodsServerHost) >= 0) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"xmsgHost:  %s", tmpRodsServerHost->hostName->name);
#else /* IRODS_SYSLOG */
        fprintf (stderr, "xmsgHost:  %s\n\n",tmpRodsServerHost->hostName->name);
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_NOTICE,"xmsgHost:  %s", tmpRodsServerHost->hostName->name);
#endif
    }

    return (0);
}

int
initRcatServerHostByFile (rsComm_t *rsComm)
{
    FILE *fptr;
    char *rcatCongFile;
    char inbuf[MAX_NAME_LEN];
    rodsHostAddr_t addr;
    rodsServerHost_t *tmpRodsServerHost;
    int lineLen, bytesCopied, remoteFlag;
    char keyWdName[MAX_NAME_LEN];
    int gptRcatFlag = 0;
    int remoteSidCount = 0;
    char sidKey[MAX_PASSWORD_LEN]="";
    int i;

    localSID[0]='\0';
    for (i=0;i<MAX_FED_RSIDS;i++) {
       remoteSID[i][0]='\0';
    }

    rcatCongFile =  (char *) malloc((strlen (getConfigDir()) +
        strlen(RCAT_HOST_FILE) + 24));

#ifndef windows_platform
    sprintf (rcatCongFile, "%-s/%-s", getConfigDir(), RCAT_HOST_FILE);
    fptr = fopen (rcatCongFile, "r");
#else
	sprintf(rcatCongFile, "%s\\%s", getConfigDir(),RCAT_HOST_FILE);
	fptr = iRODSNt_fopen(rcatCongFile, "r");
#endif

    if (fptr == NULL) {
        rodsLog (LOG_SYS_FATAL, 
	  "Cannot open RCAT_HOST_FILE  file %s. ernro = %d\n",
          rcatCongFile, errno);
        free (rcatCongFile);
	return (SYS_CONFIG_FILE_ERR);
    }

    free (rcatCongFile);

    memset (&addr, 0, sizeof (addr));
    while ((lineLen = getLine (fptr, inbuf, MAX_NAME_LEN)) > 0) {
	char *inPtr = inbuf;
	if ((bytesCopied = getStrInBuf (&inPtr, keyWdName, 
	  &lineLen, LONG_NAME_LEN)) > 0) {
	    /* advance inPtr */
	    if (strcmp (keyWdName, RE_RULESET_KW) == 0) { 
		if ((bytesCopied = getStrInBuf (&inPtr, reRuleStr,
          	 &lineLen, LONG_NAME_LEN)) < 0) {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
		    return (SYS_CONFIG_FILE_ERR);
		}
	    } else if (strcmp (keyWdName, RE_FUNCMAPSET_KW) == 0) { 
		if ((bytesCopied = getStrInBuf (&inPtr, reFuncMapStr,
          	 &lineLen, LONG_NAME_LEN)) < 0) {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
		    return (SYS_CONFIG_FILE_ERR);
		}
	    } else if (strcmp (keyWdName, RE_VARIABLEMAPSET_KW) == 0) { 
		if ((bytesCopied = getStrInBuf (&inPtr, reVariableMapStr,
          	 &lineLen, LONG_NAME_LEN)) < 0) {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
		    return (SYS_CONFIG_FILE_ERR);
		}
	    } else if (strcmp (keyWdName, KERBEROS_NAME_KW) == 0) {
               if ((bytesCopied = getStrInBuf (&inPtr, KerberosName,
	          &lineLen, LONG_NAME_LEN)) < 0) {
                     rodsLog (LOG_SYS_FATAL,
                        "initRcatServerHostByFile: parsing error for keywd %s",
                        keyWdName);
		     return (SYS_CONFIG_FILE_ERR);
               }
	    } else if (strcmp (keyWdName, ICAT_HOST_KW) == 0) { 
		if ((bytesCopied = getStrInBuf (&inPtr, addr.hostAddr,
          	 &lineLen, LONG_NAME_LEN)) > 0) {
    		    remoteFlag = resolveHost (&addr, &tmpRodsServerHost);
		    if (remoteFlag < 0) {
		        rodsLog (LOG_SYS_FATAL,
		          "initRcatServerHostByFile: resolveHost error for %s, status = %d",
		          addr.hostAddr, remoteFlag);
		        return (remoteFlag);
		    }
		    tmpRodsServerHost->rcatEnabled = LOCAL_ICAT;
		    gptRcatFlag = 1;
		} else {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
		    return (SYS_CONFIG_FILE_ERR);
		}
            } else if (strcmp (keyWdName, RE_HOST_KW) == 0) {
                if ((bytesCopied = getStrInBuf (&inPtr, addr.hostAddr,
                 &lineLen, LONG_NAME_LEN)) > 0) {
                    remoteFlag = resolveHost (&addr, &tmpRodsServerHost);
                    if (remoteFlag < 0) {
                        rodsLog (LOG_SYS_FATAL,
                          "initRcatServerHostByFile: resolveHost error for %s, status = %d",
                          addr.hostAddr, remoteFlag);
                        return (remoteFlag);
                    }
                    tmpRodsServerHost->reHostFlag = 1;
                } else {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
                    return (SYS_CONFIG_FILE_ERR);
                }
            } else if (strcmp (keyWdName, XMSG_HOST_KW) == 0) {
                if ((bytesCopied = getStrInBuf (&inPtr, addr.hostAddr,
                 &lineLen, LONG_NAME_LEN)) > 0) {
                    remoteFlag = resolveHost (&addr, &tmpRodsServerHost);
                    if (remoteFlag < 0) {
                        rodsLog (LOG_SYS_FATAL,
                          "initRcatServerHostByFile: resolveHost error for %s, status = %d",
                          addr.hostAddr, remoteFlag);
                        return (remoteFlag);
                    }
                    tmpRodsServerHost->xmsgHostFlag = 1;
                } else {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
                    return (SYS_CONFIG_FILE_ERR);
                }
	    } else if (strcmp (keyWdName, SLAVE_ICAT_HOST_KW) == 0) {
                if ((bytesCopied = getStrInBuf (&inPtr, addr.hostAddr,
                 &lineLen, LONG_NAME_LEN)) > 0) {
                    remoteFlag = resolveHost (&addr, &tmpRodsServerHost);
                    if (remoteFlag < 0) {
                        rodsLog (LOG_SYS_FATAL,
                          "initRcatServerHostByFile: resolveHost error for %s, status = %d",
                          addr.hostAddr, remoteFlag);
                        return (remoteFlag);
                    }
		    tmpRodsServerHost->rcatEnabled = LOCAL_SLAVE_ICAT;
                } else {
                    rodsLog (LOG_SYS_FATAL,
                      "initRcatServerHostByFile: parsing error for keywd %s",
                       keyWdName);
                    return (SYS_CONFIG_FILE_ERR);
		}
	    } else if (strcmp(keyWdName, LOCAL_ZONE_SID_KW) == 0) {
	       getStrInBuf(&inPtr, localSID, &lineLen, MAX_PASSWORD_LEN);
	    } else if (strcmp(keyWdName, REMOTE_ZONE_SID_KW) == 0) {
	       if (remoteSidCount < MAX_FED_RSIDS) {
		  getStrInBuf(&inPtr, remoteSID[remoteSidCount],
			      &lineLen, MAX_PASSWORD_LEN);
		  remoteSidCount++;
	       }
	    } else if (strcmp(keyWdName, SID_KEY_KW) == 0) {
	       getStrInBuf(&inPtr, sidKey, &lineLen, MAX_PASSWORD_LEN);
	    }
	}
    } 
    fclose (fptr);

    /* Possibly descramble the Server ID strings */
    if (strlen(sidKey) > 0) {
        char SID[MAX_PASSWORD_LEN+10];
        int i;
        if (strlen(localSID) > 0) {
	     strncpy(SID, localSID, MAX_PASSWORD_LEN);
	     obfDecodeByKey(SID, sidKey, localSID);
	}
	for (i=0;i<MAX_FED_RSIDS;i++) {
	    if (strlen(remoteSID[i]) > 0) {
	        strncpy(SID, remoteSID[i], MAX_PASSWORD_LEN);
		obfDecodeByKey(SID, 
			       sidKey,
			       remoteSID[i]);
	    }
	    else {
	        break;
	    }
	}
    }

    if (gptRcatFlag <= 0) {
       rodsLog (LOG_SYS_FATAL,
          "initRcatServerHostByFile: icatHost entry missing in %s.\n",
          RCAT_HOST_FILE);
        return (SYS_CONFIG_FILE_ERR);
    }

    return (0);
}

int
queAddr (rodsServerHost_t *rodsServerHost, char *myHostName)
{
    struct hostent *hostEnt;
    time_t beforeTime, afterTime;
    int status;

    if (rodsServerHost == NULL || myHostName == NULL) {
	return (0);
    }

    /* gethostbyname could hang for some address */

    beforeTime = time (0);
    if ((hostEnt = gethostbyname (myHostName)) == NULL) {
	status = SYS_GET_HOSTNAME_ERR - errno;
        if (ProcessType == SERVER_PT) {
	    rodsLog (LOG_NOTICE,
              "queAddr: gethostbyname error for %s ,errno = %d\n",
              myHostName, errno);
	}
        return (status);
    }
    afterTime = time (0);
    if (afterTime - beforeTime >= 2) {
        rodsLog (LOG_NOTICE,
         "WARNING WARNING: gethostbyname of %s is taking %d sec. This could severely affect interactivity of your Rods system",
         myHostName, afterTime - beforeTime);
	/* XXXXXX may want to mark resource down later */
    }

    if (strcasecmp (myHostName, hostEnt->h_name) != 0) {
        queHostName (rodsServerHost, hostEnt->h_name, 0);
    }
    return (0);
}

int 
queHostName (rodsServerHost_t *rodsServerHost, char *myName, int topFlag)
{
    hostName_t *myHostName, *lastHostName;
    hostName_t *tmpHostName;

    myHostName = lastHostName = rodsServerHost->hostName;
    
    while (myHostName != NULL) {
	if (strcmp (myName, myHostName->name) == 0) {
	    return (0);
	}
	lastHostName = myHostName;
	myHostName = myHostName->next;
    }
    
    tmpHostName = (hostName_t*)malloc (sizeof (hostName_t));
    tmpHostName->name = strdup (myName);
 
    if (topFlag > 0) {
	tmpHostName->next = rodsServerHost->hostName;
	rodsServerHost->hostName = tmpHostName;
    } else {
        if (lastHostName == NULL) {
	    rodsServerHost->hostName = tmpHostName;
        } else {
	    lastHostName->next = tmpHostName;
	}
        tmpHostName->next = NULL;
    }

    return (0);
}

int
queRodsServerHost (rodsServerHost_t **rodsServerHostHead, 
rodsServerHost_t *myRodsServerHost)
{
    rodsServerHost_t *lastRodsServerHost, *tmpRodsServerHost;

    lastRodsServerHost = tmpRodsServerHost = *rodsServerHostHead;
    while (tmpRodsServerHost != NULL) {
        lastRodsServerHost = tmpRodsServerHost;
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
  
    if (lastRodsServerHost == NULL) {
	*rodsServerHostHead = myRodsServerHost;
    } else {
        lastRodsServerHost->next = myRodsServerHost;
    }
    myRodsServerHost->next = NULL;

    return (0);
}

char *
getConfigDir()
{
#ifndef windows_platform
    char *myDir;

    if ((myDir = (char *) getenv("irodsConfigDir")) != (char *) NULL) {
        return (myDir);
    }
    return (DEF_CONFIG_DIR);
#else
	return iRODSNtGetServerConfigPath();
#endif
}

char *
getLogDir()
{
#ifndef windows_platform
    char *myDir;

    if ((myDir = (char *) getenv("irodsLogDir")) != (char *) NULL) {
        return (myDir);
    }
    return (DEF_LOG_DIR);
#else
	return iRODSNtServerGetLogDir;
#endif
}

/* getAndConnRcatHost - get the rcat enabled host (result given in
 * rodsServerHost) based on the rcatZoneHint.  
 * rcatZoneHint is the hint for which zone to go it. It can be :
 *	a full path - e.g., /A/B/C. In this case, "A" will be taken as the zone
 *	a zone name - a string wth the first character that is no '/' is taken
 *	   as a zone name.
 *	NULL string - default to local zone
 * If the rcat host is remote, it will automatically connect to the rcat host.
 */

int 
getAndConnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost)
{
    int status;

    status = getRcatHost (rcatType, rcatZoneHint, rodsServerHost);

    if (status < 0) {
	rodsLog (LOG_NOTICE, 
	 "getAndConnRcatHost:getRcatHost() failed. erro=%d", status);
	return (status);
    }

    if ((*rodsServerHost)->localFlag == LOCAL_HOST) {
	return (LOCAL_HOST);
    }
 
    status = svrToSvrConnect (rsComm, *rodsServerHost); 

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "getAndConnRcatHost: svrToSvrConnect to %s failed",
	  (*rodsServerHost)->hostName->name);
	if ((*rodsServerHost)->rcatEnabled == REMOTE_ICAT)
	    status = convZoneSockError (status);
    }
    if (status >= 0) {
	return (REMOTE_HOST);
    } else {
        return (status);
    }
}

int
getAndConnRcatHostNoLogin (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost)
{
    int status;

    status = getRcatHost (rcatType, rcatZoneHint, rodsServerHost);

    if (status < 0) {
        return (status);
    }

    if ((*rodsServerHost)->localFlag == LOCAL_HOST) {
        return (LOCAL_HOST);
    }

    status = svrToSvrConnectNoLogin (rsComm, *rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "getAndConnRcatHost: svrToSvrConnectNoLogin to %s failed",
          (*rodsServerHost)->hostName->name);
        if ((*rodsServerHost)->rcatEnabled == REMOTE_ICAT)
            status = convZoneSockError (status);
    }
    return (status);
}

int
convZoneSockError (int inStatus)
{
    int unixErr = getErrno (inStatus);
    if (inStatus + unixErr == USER_SOCK_CONNECT_ERR)  
	return (CROSS_ZONE_SOCK_CONNECT_ERR - unixErr);
    else 
	return inStatus;
}

/* getRcatHost - get the rodsServerHost of the rcat enable host based
 * on the rcatZoneHint for identifying the zone.
 *   rcatZoneHint == NULL ==> local rcat zone
 *   rcatZoneHint in the form of a full path,e.g.,/x/y/z ==> x is the zoneName.
 *   rcatZoneHint not in the form of a full path ==> rcatZoneHint is the zone.
 */

int
getZoneInfo (char *rcatZoneHint, zoneInfo_t **myZoneInfo) 
{
    int status;
    zoneInfo_t *tmpZoneInfo;
    int zoneInput;
    char zoneName[NAME_LEN];

    if (rcatZoneHint == NULL || strlen (rcatZoneHint) == 0) {
        zoneInput = 0;
    } else {
        zoneInput = 1;
        getZoneNameFromHint (rcatZoneHint, zoneName, NAME_LEN);
    }

    *myZoneInfo = NULL;
    tmpZoneInfo = ZoneInfoHead;
    while (tmpZoneInfo != NULL) {
        if (zoneInput == 0) {   /* assume local */
            if (tmpZoneInfo->masterServerHost->rcatEnabled == LOCAL_ICAT) {
                *myZoneInfo = tmpZoneInfo;
            }
        } else {        /* remote zone */
            if (strcmp (zoneName, tmpZoneInfo->zoneName) == 0) {
                *myZoneInfo = tmpZoneInfo;
            }
        }
	if (*myZoneInfo != NULL) return 0;
        tmpZoneInfo = tmpZoneInfo->next;
    }

    if (zoneInput == 0) {
        rodsLog (LOG_ERROR,
          "getRcatHost: No local Rcat");
        return (SYS_INVALID_ZONE_NAME);
    } else {
        rodsLog (LOG_DEBUG,
          "getZoneInfo: Invalid zone name from hint %s", rcatZoneHint);
	status = getZoneInfo (NULL, myZoneInfo);
        if (status < 0) {
            return (SYS_INVALID_ZONE_NAME);
        } else {
            return (0);
        }
    }
}

int
getRcatHost (int rcatType, char *rcatZoneHint,  
rodsServerHost_t **rodsServerHost)
{
    int status;
    zoneInfo_t *myZoneInfo = NULL;

    status = getZoneInfo (rcatZoneHint, &myZoneInfo);
    if (status < 0) return status;

    if (rcatType == MASTER_RCAT ||
      myZoneInfo->slaveServerHost == NULL) {
        *rodsServerHost = myZoneInfo->masterServerHost;
        return (myZoneInfo->masterServerHost->localFlag);
    } else {
        *rodsServerHost = myZoneInfo->slaveServerHost;
        return (myZoneInfo->slaveServerHost->localFlag);
    }
}

int
getLocalZoneInfo (zoneInfo_t **outZoneInfo)
{
    zoneInfo_t *tmpZoneInfo;

    tmpZoneInfo = ZoneInfoHead;
    while (tmpZoneInfo != NULL) {
        if (tmpZoneInfo->masterServerHost->rcatEnabled == LOCAL_ICAT) {
            *outZoneInfo = tmpZoneInfo;
	    return (0);
	}
        tmpZoneInfo = tmpZoneInfo->next;
    }
    rodsLog (LOG_ERROR,
     "getLocalZoneInfo: Local Zone does not exist");

    *outZoneInfo = NULL;
    return (SYS_INVALID_ZONE_NAME);
}

char*
getLocalZoneName ()
{
    zoneInfo_t *tmpZoneInfo;

    if (getLocalZoneInfo (&tmpZoneInfo) >= 0) {
	return (tmpZoneInfo->zoneName);
    } else {
	return NULL;
    }
}

/* Check if there is a connected ICAT host, and if there is, disconnect */
int 
getAndDisconnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost)
{
    int status;

    status = getRcatHost (rcatType, rcatZoneHint, rodsServerHost);

    if (status < 0) return(status);

    if ((*rodsServerHost)->conn != NULL) { /* a connection exists */
       status = rcDisconnect((*rodsServerHost)->conn);
       return(status);
    }
    return(0);
}

int
disconnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = getRcatHost (rcatType, rcatZoneHint, &rodsServerHost);

    if (status < 0) {
        return (status);
    }

    if ((rodsServerHost)->localFlag == LOCAL_HOST) {
        return (LOCAL_HOST);
    }

    if (rodsServerHost->conn != NULL) { /* a connection exists */
        status = rcDisconnect(rodsServerHost->conn);
	rodsServerHost->conn = NULL;
    }
    if (status >= 0) {
	return (REMOTE_HOST);
    } else {
        return (status);
    }
}

/* resetRcatHost is similar to disconnRcatHost except it does not disconnect */

int
resetRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint)
{
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = getRcatHost (rcatType, rcatZoneHint, &rodsServerHost);

    if (status < 0) {
        return (status);
    }

    if ((rodsServerHost)->localFlag == LOCAL_HOST) {
        return (LOCAL_HOST);
    }

    if (rodsServerHost->conn != NULL) { /* a connection exists */
	rodsServerHost->conn = NULL;
    }
    if (status >= 0) {
	return (REMOTE_HOST);
    } else {
        return (status);
    }
}

int
initZone (rsComm_t *rsComm)
{
    rodsEnv *myEnv = &rsComm->myEnv;
    rodsServerHost_t *tmpRodsServerHost;
    rodsServerHost_t *masterServerHost = NULL;
    rodsServerHost_t *slaveServerHost = NULL;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status, i;
    sqlResult_t *zoneName, *zoneType, *zoneConn, *zoneComment;
    char *tmpZoneName, *tmpZoneType, *tmpZoneConn, *tmpZoneComment;

    /* configure the local zone first or rsGenQuery would not work */

    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
        if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT) {
	    tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            masterServerHost = tmpRodsServerHost;
        } else if (tmpRodsServerHost->rcatEnabled == LOCAL_SLAVE_ICAT) {
	    tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            slaveServerHost = tmpRodsServerHost;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    ZoneInfoHead->masterServerHost = masterServerHost;
    ZoneInfoHead->slaveServerHost = slaveServerHost;
    /* queZone (myEnv->rodsZone, masterServerHost, slaveServerHost); */

    memset (&genQueryInp, 0, sizeof (genQueryInp));
    addInxIval (&genQueryInp.selectInp, COL_ZONE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_ZONE_TYPE, 1);
    addInxIval (&genQueryInp.selectInp, COL_ZONE_CONNECTION, 1);
    addInxIval (&genQueryInp.selectInp, COL_ZONE_COMMENT, 1);
    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

    clearGenQueryInp (&genQueryInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "initZone: rsGenQuery error, status = %d", status);
        return (status);
    }

    if (genQueryOut == NULL) {
        rodsLog (LOG_NOTICE,
          "initZone: NULL genQueryOut");
        return (CAT_NO_ROWS_FOUND);
    }

    if ((zoneName = getSqlResultByInx (genQueryOut, COL_ZONE_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
          "initZone: getSqlResultByInx for COL_ZONE_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((zoneType = getSqlResultByInx (genQueryOut, COL_ZONE_TYPE)) == NULL) {
        rodsLog (LOG_NOTICE,
          "initZone: getSqlResultByInx for COL_ZONE_TYPE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((zoneConn = getSqlResultByInx (genQueryOut, COL_ZONE_CONNECTION)) == NULL) {
        rodsLog (LOG_NOTICE,
          "initZone: getSqlResultByInx for COL_ZONE_CONNECTION failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((zoneComment = getSqlResultByInx (genQueryOut, COL_ZONE_COMMENT)) == NULL) {
        rodsLog (LOG_NOTICE,
          "initZone: getSqlResultByInx for COL_ZONE_COMMENT failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0;i < genQueryOut->rowCnt; i++) {
	rodsHostAddr_t addr;

        tmpZoneName = &zoneName->value[zoneName->len * i];
        tmpZoneType = &zoneType->value[zoneType->len * i];
        tmpZoneConn = &zoneConn->value[zoneConn->len * i];
        tmpZoneComment = &zoneComment->value[zoneComment->len * i];
	if (strcmp (tmpZoneType, "local") == 0) {
	    if (strcmp (myEnv->rodsZone, tmpZoneName) != 0) {
                rodsLog (LOG_ERROR,
                  "initZone: zoneName in env %s does not match %s in icat ",
		  myEnv->rodsZone, tmpZoneName);
	    }
	    /* fillin rodsZone if it is not defined */
            if (strlen (rsComm->proxyUser.rodsZone) == 0) 
                rstrcpy (rsComm->proxyUser.rodsZone, tmpZoneName, NAME_LEN);
            if (strlen (rsComm->clientUser.rodsZone) == 0) 
                rstrcpy (rsComm->clientUser.rodsZone, tmpZoneName, NAME_LEN);
	    continue;
	} else if (strlen (tmpZoneConn) <= 0) {
            rodsLog (LOG_ERROR,
              "initZone: connection info for zone %s not configured",
               tmpZoneName);
	    continue;
	}
 
        memset (&addr, 0, sizeof (addr));
	/* assume address:port */
        parseHostAddrStr (tmpZoneConn, &addr);
        if (addr.portNum == 0) addr.portNum = ZoneInfoHead->portNum;
        status = resolveHost (&addr, &tmpRodsServerHost);
	if (status < 0) {
	    rodsLog (LOG_ERROR,
              "initZone: resolveHost error for %s for zone %s. status = %d",
               addr.hostAddr, tmpZoneName, status);
	    continue;
	}
	if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT) { 
	    rodsLog (LOG_ERROR,
              "initZone: address %s for remote zone %s already in use",
               tmpZoneConn, tmpZoneName);
	    continue;
	}
	tmpRodsServerHost->rcatEnabled = REMOTE_ICAT;
        queZone (tmpZoneName, addr.portNum, tmpRodsServerHost, NULL);
    }

    freeGenQueryOut (&genQueryOut);

    return (0); 
}

int
queZone (char *zoneName, int portNum, rodsServerHost_t *masterServerHost,
rodsServerHost_t *slaveServerHost) 
{
    zoneInfo_t *tmpZoneInfo, *lastZoneInfo;
    zoneInfo_t *myZoneInfo;

    myZoneInfo = (zoneInfo_t *) malloc (sizeof (zoneInfo_t));

    memset (myZoneInfo, 0, sizeof (zoneInfo_t));

    rstrcpy (myZoneInfo->zoneName, zoneName, NAME_LEN);
    if (masterServerHost != NULL) {
        myZoneInfo->masterServerHost = masterServerHost;
	masterServerHost->zoneInfo = myZoneInfo;
    }
    if (slaveServerHost != NULL) {
        myZoneInfo->slaveServerHost = slaveServerHost;
	slaveServerHost->zoneInfo = myZoneInfo;
    }

    if (portNum <= 0) {
	if (ZoneInfoHead != NULL) {
	    myZoneInfo->portNum = ZoneInfoHead->portNum; 
	} else {
            rodsLog (LOG_ERROR,
              "queZone:  Bad input portNum %d for %s", portNum, zoneName);
            free (myZoneInfo);
            return (SYS_INVALID_SERVER_HOST);
        }
    } else {
	myZoneInfo->portNum = portNum;
    }

    /* queue it */

    lastZoneInfo = tmpZoneInfo = ZoneInfoHead;
    while (tmpZoneInfo != NULL) {
        lastZoneInfo = tmpZoneInfo;
        tmpZoneInfo = tmpZoneInfo->next;
    }

    if (lastZoneInfo == NULL) {
        ZoneInfoHead = myZoneInfo;
    } else {
        lastZoneInfo->next = myZoneInfo;
    }
    myZoneInfo->next = NULL;

    if (masterServerHost == NULL) {
        rodsLog (LOG_DEBUG,
          "queZone:  masterServerHost for %s is NULL", zoneName);
        return (SYS_INVALID_SERVER_HOST);
    } else {
        return (0);
    }
}

int 
setExecArg (char *commandArgv, char *av[])
{
    char *inpPtr, *outPtr;
    int inx = 1;
    int c;
    int len = 0;
    int openQuote = 0;

    if (commandArgv != NULL) {
      inpPtr = strdup (commandArgv);
      outPtr = inpPtr;
      while ((c = *inpPtr) != '\0') {
        if ((c == ' ' && openQuote == 0) || openQuote == 2) {
            openQuote = 0;
            if (len > 0) {      /* end of a argv */
                *inpPtr = '\0'; /* mark the end */
                av[inx] = outPtr;
                /* reset inx and pointer */
                inpPtr++;
                outPtr = inpPtr;
                inx ++;
                len = 0;
            } else {    /* skip over blanks */
                inpPtr++;
                outPtr = inpPtr;
            }
        } else if (c == '\'' || c == '\"') {
            openQuote ++;
            if (openQuote == 1) {
                /* skip the quote */
                inpPtr++;
                outPtr = inpPtr;
            }
        } else {
            len ++;
            inpPtr++;
        }
      }
      if (len > 0) {      /* handle the last argv */
        av[inx] = outPtr;
        inx++;
      }
    }

    av[inx] = NULL;     /* terminate with a NULL */

    return (0);
}
#ifdef RULE_ENGINE_N
int
initAgent (int processType, rsComm_t *rsComm)
#else
int
initAgent (rsComm_t *rsComm)
#endif
{
    int status;
    rsComm_t myComm;
    ruleExecInfo_t rei;

    initProcLog ();

    status = initServerInfo (rsComm);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "initAgent: initServerInfo error, status = %d",
          status);
        return (status);
    }

    initL1desc ();
    initSpecCollDesc ();
    initCollHandle ();
    status = initFileDesc ();
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "initAgent: initFileDesc error, status = %d",
          status);
        return (status);
    }
#ifdef TAR_STRUCT_FILE
    initStructFileDesc ();
    initTarSubFileDesc ();
#endif

#ifdef RULE_ENGINE_N
    status = initRuleEngine(processType, rsComm, reRuleStr, reFuncMapStr, reVariableMapStr);
#else
    status = initRuleEngine(rsComm, reRuleStr, reFuncMapStr, reVariableMapStr);
#endif
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "initAgent: initRuleEngine error, status = %d", status);
        return(status);
    }

    memset (&rei, 0, sizeof (rei));
    rei.rsComm = rsComm;

    if (ProcessType == AGENT_PT) {
        status = applyRule ("acChkHostAccessControl", NULL, &rei, 
	  NO_SAVE_REI);

        if (status < 0) {
            rodsLog (LOG_ERROR,
              "initAgent: acChkHostAccessControl error, status = %d",
              status);
            return (status);
	}
    }

    /* Initialize the global quota */

    GlobalQuotaLimit = RESC_QUOTA_UNINIT;
    GlobalQuotaOverrun = 0;
    RescQuotaPolicy = RESC_QUOTA_UNINIT;	

    seedRandom ();

#ifndef windows_platform
    if (rsComm->reconnFlag == RECONN_TIMEOUT) { 
	rsComm->reconnSock = svrSockOpenForInConn (rsComm, &rsComm->reconnPort,
	  &rsComm->reconnAddr, SOCK_STREAM);
	if (rsComm->reconnSock < 0) {
	    rsComm->reconnPort = 0;
	    rsComm->reconnAddr = NULL;
        } else {
	    rsComm->cookie = random ();
	}
#ifdef USE_BOOST
	rsComm->lock = new boost::mutex;
	rsComm->cond = new boost::condition_variable;
	rsComm->reconnThr = new boost::thread( reconnManager, rsComm );
#else
	pthread_mutex_init (&rsComm->lock, NULL);
	pthread_cond_init (&rsComm->cond, NULL);
        status = pthread_create  (&rsComm->reconnThr, pthread_attr_default,
              (void *(*)(void *)) reconnManager,
              (void *) rsComm);
#endif
        if (status < 0) {
            rodsLog (LOG_ERROR, "initAgent: pthread_create failed, stat=%d",
	      status);
	}
    }
    initExecCmdMutex ();
#endif

    InitialState = INITIAL_DONE;
    ThisComm = rsComm;

    /* use a tmp myComm is needed to get the permission right for the call */
    myComm = *rsComm;
    myComm.clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rei.rsComm = &myComm;

    status = applyRule ("acSetPublicUserPolicy", NULL, &rei, NO_SAVE_REI);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "initAgent: acSetPublicUserPolicy error, status = %d",
          status);
        return (status);
    }

    return (status);
}

void
cleanupAndExit (int status)
{
    rodsLog (LOG_NOTICE,
      "Agent exiting with status = %d", status);

#ifdef RODS_CAT
    disconnectRcat (ThisComm);
#endif

    /* added by RAJA April 12, 2011 */
    finalzeRuleEngine(ThisComm);

    if (InitialState == INITIAL_DONE) {
	/* close all opened descriptors */
	closeAllL1desc (ThisComm);
        /* close any opened server to server connection */
	disconnectAllSvrToSvrConn ();
    }


    if (status >= 0) {
        exit (0);
    } else {
        exit (1);
    }
}

void
#ifdef  __cplusplus
signalExit (int)
#else
signalExit ()
#endif
{
    rodsLog (LOG_NOTICE,
     "caught a signal and exiting\n");
    cleanupAndExit (SYS_CAUGHT_SIGNAL);
}

void
#ifdef  __cplusplus
rsPipSigalHandler (int)
#else
rsPipSigalHandler ()
#endif
{
    time_t curTime;

    if (ThisComm == NULL || ThisComm->reconnSock <= 0) {
	rodsLog (LOG_NOTICE,
         "caught a broken pipe signal and exiting");
        cleanupAndExit (SYS_CAUGHT_SIGNAL);
    } else {
	curTime = time (0);
	if (curTime - LastBrokenPipeTime < BROKEN_PIPE_INT) {
	    BrokenPipeCnt ++;
	    if (BrokenPipeCnt > MAX_BROKEN_PIPE_CNT) {
		rodsLog (LOG_NOTICE,
         	  "caught a broken pipe signal and exiting");
		cleanupAndExit (SYS_CAUGHT_SIGNAL);
	    }
	} else {
	    BrokenPipeCnt = 1;
	}
	LastBrokenPipeTime = curTime;
	rodsLog (LOG_NOTICE,
         "caught a broken pipe signal. Attempt to reconnect");
#ifndef _WIN32
        signal(SIGPIPE, (void (*)(int)) rsPipSigalHandler);
#endif

    }
}

int
initHostConfigByFile (rsComm_t *rsComm)
{
    FILE *fptr;
    char *hostCongFile;
    char inbuf[MAX_NAME_LEN];
    char hostBuf[LONG_NAME_LEN];
    rodsServerHost_t *tmpRodsServerHost;
    int lineLen, bytesCopied;
    int status;

    hostCongFile =  (char *) malloc((strlen (getConfigDir()) +
        strlen(HOST_CONFIG_FILE) + 24));

#ifndef windows_platform
    sprintf (hostCongFile, "%-s/%-s", getConfigDir(), HOST_CONFIG_FILE);
	fptr = fopen (hostCongFile, "r");
#else
	sprintf(hostCongFile, "%s\\%s", getConfigDir(),HOST_CONFIG_FILE);
	fptr = iRODSNt_fopen(hostCongFile, "r");
#endif

    if (fptr == NULL) {
        rodsLog (LOG_NOTICE,
          "Cannot open HOST_CONFIG_FILE  file %s. ernro = %d\n",
          hostCongFile, errno);
        free (hostCongFile);
        return (SYS_CONFIG_FILE_ERR);
    }

    free (hostCongFile);

    while ((lineLen = getLine (fptr, inbuf, MAX_NAME_LEN)) > 0) {
        char *inPtr = inbuf;
	int cnt = 0;
        while ((bytesCopied = getStrInBuf (&inPtr, hostBuf,
          &lineLen, LONG_NAME_LEN)) > 0) {
	    if (cnt == 0) {
		/* first host */
		tmpRodsServerHost = (rodsServerHost_t*)malloc (sizeof (rodsServerHost_t));
                memset (tmpRodsServerHost, 0, sizeof (rodsServerHost_t));
		/* assume it is remote */
		tmpRodsServerHost->localFlag = REMOTE_HOST;
		/* local zone */
		tmpRodsServerHost->zoneInfo = ZoneInfoHead;
		status = queRodsServerHost (&HostConfigHead, tmpRodsServerHost);

	    }
	    cnt ++;
	    if (strcmp (hostBuf, "localhost") == 0) {
		tmpRodsServerHost->localFlag = LOCAL_HOST;
	    } else {
                status = queHostName (tmpRodsServerHost, hostBuf, 0);
	    }
        }
    }
    fclose (fptr);
    return (0);
}

int
matchHostConfig (rodsServerHost_t *myRodsServerHost)
{
    rodsServerHost_t *tmpRodsServerHost;
    int status;

    if (myRodsServerHost == NULL)
	return (0);

    if (myRodsServerHost->localFlag == LOCAL_HOST) {
        tmpRodsServerHost = HostConfigHead;
	while (tmpRodsServerHost != NULL) {
	    if (tmpRodsServerHost->localFlag == LOCAL_HOST) {
		status = queConfigName (tmpRodsServerHost, myRodsServerHost);
		return (status);
	    }
	    tmpRodsServerHost = tmpRodsServerHost->next;
	}
    } else {
        tmpRodsServerHost = HostConfigHead;
        while (tmpRodsServerHost != NULL) {
	    hostName_t *tmpHostName, *tmpConfigName;

            if (tmpRodsServerHost->localFlag == LOCAL_HOST &&
	      myRodsServerHost->localFlag != UNKNOWN_HOST_LOC) {
		tmpRodsServerHost = tmpRodsServerHost->next;
		continue;
	    }
	
            tmpConfigName = tmpRodsServerHost->hostName;
            while (tmpConfigName != NULL) {
                tmpHostName = myRodsServerHost->hostName;
                while (tmpHostName != NULL) {
		    if (strcmp (tmpHostName->name, tmpConfigName->name) == 0) {
			myRodsServerHost->localFlag = 
			  tmpRodsServerHost->localFlag;
		        status = queConfigName (tmpRodsServerHost,
		          myRodsServerHost);
                        return (0);
		    }
		    tmpHostName = tmpHostName->next;
		}
		tmpConfigName = tmpConfigName->next;
            }
            tmpRodsServerHost = tmpRodsServerHost->next;
        }
    }
	
    return (0);
}

int
queConfigName (rodsServerHost_t *configServerHost, 
rodsServerHost_t *myRodsServerHost)
{ 
    hostName_t *tmpHostName = configServerHost->hostName;
    int cnt = 0;

    while (tmpHostName != NULL) {
        if (cnt == 0) {
            /* queue the first one on top */
            queHostName (myRodsServerHost, tmpHostName->name, 1);
        } else {
            queHostName (myRodsServerHost, tmpHostName->name, 0);
        }
        cnt ++;
        tmpHostName = tmpHostName->next;
    }

    return (0);
}

int
disconnectAllSvrToSvrConn ()
{
    rodsServerHost_t *tmpRodsServerHost;

    /* check if host exist */

    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
	if (tmpRodsServerHost->conn != NULL) {
	    rcDisconnect (tmpRodsServerHost->conn);
	    tmpRodsServerHost->conn = NULL;
	}
	tmpRodsServerHost = tmpRodsServerHost->next;
    }
    return (0);
}

int
initRsComm (rsComm_t *rsComm)
{
    int status;

    memset (rsComm, 0, sizeof (rsComm_t));
    status = getRodsEnv (&rsComm->myEnv);


    if (status < 0) {
        rodsLog (LOG_ERROR,
          "initRsComm: getRodsEnv serror, status = %d", status);
        return (status);
    }

    /* fill in the proxyUser info from myEnv. clientUser has to come from
     * the rei */

    rstrcpy (rsComm->proxyUser.userName, rsComm->myEnv.rodsUserName, NAME_LEN);
    rstrcpy (rsComm->proxyUser.rodsZone, rsComm->myEnv.rodsZone, NAME_LEN);
    rstrcpy (rsComm->proxyUser.authInfo.authScheme,
      rsComm->myEnv.rodsAuthScheme, NAME_LEN);
    rstrcpy (rsComm->clientUser.userName, rsComm->myEnv.rodsUserName, NAME_LEN);
    rstrcpy (rsComm->clientUser.rodsZone, rsComm->myEnv.rodsZone, NAME_LEN);
    rstrcpy (rsComm->clientUser.authInfo.authScheme,
      rsComm->myEnv.rodsAuthScheme, NAME_LEN);
    /* assume LOCAL_PRIV_USER_AUTH */
    rsComm->clientUser.authInfo.authFlag =
     rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

    return (0);
}

void
daemonize (int runMode, int logFd)
{
#ifndef _WIN32
    if (runMode == SINGLE_PASS)
        return;

    if (runMode == STANDALONE_SERVER) {
        if (fork())
            exit (0);

        if (setsid() < 0) {
            fprintf(stderr, "daemonize");
            perror("cannot create a new session.");
            exit(1);
        }
    }

    close (0);
    close (1);
    close (2);

    (void) dup2 (logFd, 0);
    (void) dup2 (logFd, 1);
    (void) dup2 (logFd, 2);
    close (logFd);
#endif
}

/* logFileOpen - Open the logFile for the reServer.
 *
 * Input - None
 * OutPut - the log file descriptor
 */

int
logFileOpen (int runMode, char *logDir, char *logFileName)
{
    char *logFile = NULL;
#ifdef IRODS_SYSLOG
    int logFd = 0;
#else
    int logFd;
#endif

    if (runMode == SINGLE_PASS && logDir == NULL) {
        return (1);
    }

    if (logFileName == NULL) {
        fprintf (stderr, "logFileOpen: NULL input logFileName\n");
	return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    getLogfileName (&logFile, logDir, logFileName);

#ifndef IRODS_SYSLOG
    logFd = open (logFile, O_CREAT|O_WRONLY|O_APPEND, 0666);
#endif
    if (logFd < 0) {
        fprintf (stderr, "logFileOpen: Unable to open %s. errno = %d\n",
          logFile, errno);
        return (-1 * errno);
    }


    return (logFd);
}

int
initRsCommWithStartupPack (rsComm_t *rsComm, startupPack_t *startupPack)
{
    char *tmpStr;
    static char tmpStr2[LONG_NAME_LEN]; /** RAJA added to take care of memory 
                                         * leak Nov 15 2010 found by J-Y **/
    /* always use NATIVE_PROT as a client. e.g., server to server comm */
    snprintf (tmpStr2, LONG_NAME_LEN, "%s=%d", IRODS_PROT, NATIVE_PROT);
    putenv (tmpStr2);

    if (startupPack != NULL) {
        rsComm->connectCnt = startupPack->connectCnt;
        rsComm->irodsProt = startupPack->irodsProt;
        rsComm->reconnFlag = startupPack->reconnFlag;
        rstrcpy (rsComm->proxyUser.userName, startupPack->proxyUser, 
	  NAME_LEN);
        if (strcmp (startupPack->proxyUser, PUBLIC_USER_NAME) == 0) {
            rsComm->proxyUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }
        rstrcpy (rsComm->proxyUser.rodsZone, startupPack->proxyRodsZone, 
          NAME_LEN);
        rstrcpy (rsComm->clientUser.userName, startupPack->clientUser, 
	  NAME_LEN);
        if (strcmp (startupPack->clientUser, PUBLIC_USER_NAME) == 0) {
            rsComm->clientUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }
        rstrcpy (rsComm->clientUser.rodsZone, startupPack->clientRodsZone, 
          NAME_LEN);
        rstrcpy (rsComm->cliVersion.relVersion, startupPack->relVersion, 
          NAME_LEN);
        rstrcpy (rsComm->cliVersion.apiVersion, startupPack->apiVersion, 
          NAME_LEN);
        rstrcpy (rsComm->option, startupPack->option, NAME_LEN);
    } else {	/* have to depend on env variable */
        tmpStr = getenv (SP_NEW_SOCK);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist", 
	      SP_NEW_SOCK);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rsComm->sock = atoi (tmpStr);

        tmpStr = getenv (SP_CONNECT_CNT);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist", 
	      SP_CONNECT_CNT);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rsComm->connectCnt = atoi (tmpStr) + 1;

        tmpStr = getenv (SP_PROTOCOL);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist", 
	      SP_PROTOCOL);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rsComm->irodsProt = (irodsProt_t)atoi (tmpStr);

        tmpStr = getenv (SP_RECONN_FLAG);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist", 
	      SP_RECONN_FLAG);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rsComm->reconnFlag = atoi (tmpStr);

        tmpStr = getenv (SP_PROXY_USER);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist", 
	      SP_PROXY_USER);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->proxyUser.userName, tmpStr, NAME_LEN);
        if (strcmp (tmpStr, PUBLIC_USER_NAME) == 0) {
            rsComm->proxyUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }

        tmpStr = getenv (SP_PROXY_RODS_ZONE);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist",
              SP_PROXY_RODS_ZONE);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->proxyUser.rodsZone, tmpStr, NAME_LEN);

        tmpStr = getenv (SP_CLIENT_USER);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist",
              SP_CLIENT_USER);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->clientUser.userName, tmpStr, NAME_LEN);
        if (strcmp (tmpStr, PUBLIC_USER_NAME) == 0) {
            rsComm->clientUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }

        tmpStr = getenv (SP_CLIENT_RODS_ZONE);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist",
              SP_CLIENT_RODS_ZONE);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->clientUser.rodsZone, tmpStr, NAME_LEN);

        tmpStr = getenv (SP_REL_VERSION);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "getstartupPackFromEnv: env %s does not exist",
              SP_REL_VERSION);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->cliVersion.relVersion, tmpStr, NAME_LEN);

        tmpStr = getenv (SP_API_VERSION);
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist",
              SP_API_VERSION);
            return (SYS_GETSTARTUP_PACK_ERR);
        }
        rstrcpy (rsComm->cliVersion.apiVersion, tmpStr, NAME_LEN);

        tmpStr = getenv (SP_OPTION);
#ifndef windows_platform
        if (tmpStr == NULL) {
            rodsLog (LOG_NOTICE,
              "initRsCommWithStartupPack: env %s does not exist",
              SP_OPTION);
        } else {
	    rstrcpy (rsComm->option, tmpStr, NAME_LEN);
	}
#else
	if(tmpStr != NULL) {
	    rstrcpy (rsComm->option, tmpStr, NAME_LEN);
	}
#endif
        
    }
    if (rsComm->sock != 0) { /* added by RAJA Nov 16 2010 to remove error 
                              * messages from xmsLog */
        setLocalAddr (rsComm->sock, &rsComm->localAddr);
        setRemoteAddr (rsComm->sock, &rsComm->remoteAddr);
    } /* added by RAJA Nov 16 2010 to remove error messages from xmsLog */

    tmpStr = inet_ntoa (rsComm->remoteAddr.sin_addr);
    if (tmpStr == NULL || *tmpStr == '\0') tmpStr = "UNKNOWN";
    rstrcpy (rsComm->clientAddr, tmpStr, NAME_LEN);

    return (0);
}

/* getAndConnRemoteZone - get the remote zone host (result given in
 * rodsServerHost) based on the dataObjInp->objPath as zoneHint.
 * If the host is a remote zone, automatically connect to the host.
 */

int
getAndConnRemoteZone (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsServerHost_t **rodsServerHost, char *remotZoneOpr)
{
    int status;

    status = getRemoteZoneHost (rsComm, dataObjInp, rodsServerHost, 
      remotZoneOpr);

    if (status == LOCAL_HOST) {
        return LOCAL_HOST;
    } else if (status < 0) {
	return status;
    }

    status = svrToSvrConnect (rsComm, *rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "getAndConnRemoteZone: svrToSvrConnect to %s failed",
          (*rodsServerHost)->hostName->name);
    }
    if (status >= 0) {
        return (REMOTE_HOST);
    } else {
        return (status);
    }
}

int
getAndConnRemoteZoneForCopy (rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp,
rodsServerHost_t **rodsServerHost)
{
    int status;
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    rodsServerHost_t *srcIcatServerHost = NULL;
    rodsServerHost_t *destIcatServerHost = NULL;

    srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    destDataObjInp = &dataObjCopyInp->destDataObjInp;

    status = getRcatHost (MASTER_RCAT, srcDataObjInp->objPath, 
      &srcIcatServerHost);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "getAndConnRemoteZoneForCopy: getRcatHost error for %s",
          srcDataObjInp->objPath);
        return (status);
    }

    if (srcIcatServerHost->rcatEnabled != REMOTE_ICAT) {
        /* local zone. nothing to do */
        return (LOCAL_HOST);
    }

    status = getRcatHost (MASTER_RCAT, destDataObjInp->objPath,
      &destIcatServerHost);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "getAndConnRemoteZoneForCopy: getRcatHost error for %s",
          destDataObjInp->objPath);
        return (status);
    }

    if (destIcatServerHost->rcatEnabled != REMOTE_ICAT) {
        /* local zone. nothing to do */
        return (LOCAL_HOST);
    }

#if 0
    if (srcIcatServerHost != destIcatServerHost) {
        return (LOCAL_HOST);
    }

    /* from the same remote zone */
#endif

    status = getAndConnRemoteZone (rsComm, destDataObjInp, rodsServerHost, 
      REMOTE_CREATE);

    return status;
}

int
isLocalZone (char *zoneHint)
{
    int status;
    rodsServerHost_t *icatServerHost = NULL;

    status = getRcatHost (MASTER_RCAT, zoneHint, &icatServerHost);

    if (status < 0) {
        return (0);
    }

    if (icatServerHost->rcatEnabled != REMOTE_ICAT) {
        /* local zone. */
        return 1;
    } else {
	return 0;
    }
}

/* isSameZone - return 1 if from same zone, otherwise return 0
 */
int
isSameZone (char *zoneHint1, char *zoneHint2)
{
    char zoneName1[NAME_LEN], zoneName2[NAME_LEN];

    if (zoneHint1 == NULL || zoneHint2 == NULL) return 0;

    getZoneNameFromHint (zoneHint1, zoneName1, NAME_LEN);
    getZoneNameFromHint (zoneHint2, zoneName2, NAME_LEN);

    if (strcmp (zoneName1, zoneName2) == 0)
	return 1;
    else
	return 0;
}

int
getRemoteZoneHost (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsServerHost_t **rodsServerHost, char *remotZoneOpr)
{
    int status;
    rodsServerHost_t *icatServerHost = NULL;
    rodsHostAddr_t *rescAddr = NULL;

    status = getRcatHost (MASTER_RCAT, dataObjInp->objPath, &icatServerHost);

    if (status < 0) {
        return (status);
    }

    if (icatServerHost->rcatEnabled != REMOTE_ICAT) {
	/* local zone. nothing to do */
        return (LOCAL_HOST);
    }

    status = svrToSvrConnect (rsComm, icatServerHost);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "getRemoteZoneHost: svrToSvrConnect to %s failed, status = %d",
          icatServerHost->hostName->name, status);
	status = convZoneSockError (status);
	return status;
    }

    addKeyVal (&dataObjInp->condInput, REMOTE_ZONE_OPR_KW, remotZoneOpr);

    status = rcGetRemoteZoneResc (icatServerHost->conn, dataObjInp, &rescAddr);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "getRemoteZoneHost: rcGetRemoteZoneResc for %s failed, status = %d",
          dataObjInp->objPath, status);
        return status;
    }

    status = resolveHost (rescAddr, rodsServerHost);

    free (rescAddr);

    return (status);
}

int
resolveAndConnHost (rsComm_t *rsComm, rodsHostAddr_t *addr, 
rodsServerHost_t **rodsServerHost)
{
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (addr, rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
	return LOCAL_HOST;
    }

    status = svrToSvrConnect (rsComm, *rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "resolveAndConnHost: svrToSvrConnect to %s failed",
          (*rodsServerHost)->hostName->name);
    }
    if (status >= 0) {
        return (REMOTE_HOST);
    } else {
        return (status);
    }
}

int
isLocalHost (char *hostAddr)
{
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    rodsHostAddr_t addr;

    bzero (&addr, sizeof (addr));
    rstrcpy (addr.hostAddr, hostAddr, NAME_LEN);
    remoteFlag = resolveHost (&addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST)
	return 1;
    else 
	return 0;
}

int
getReHost (rodsServerHost_t **rodsServerHost)
{
    int status;

    rodsServerHost_t *tmpRodsServerHost;

    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
        if (tmpRodsServerHost->reHostFlag == 1) {
	    *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    status = getRcatHost (MASTER_RCAT, NULL, rodsServerHost);

    return status;    
}

int
getXmsgHost (rodsServerHost_t **rodsServerHost)
{
    rodsServerHost_t *tmpRodsServerHost;

    tmpRodsServerHost = ServerHostHead;
    while (tmpRodsServerHost != NULL) {
        if (tmpRodsServerHost->xmsgHostFlag == 1) {
            *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    *rodsServerHost = NULL;

    return SYS_INVALID_SERVER_HOST;
}


/* getAndConnReHost - Get the irodsReServer host (result given in
 * rodsServerHost).
 * If the is remote, it will automatically connect to the server where 
 * irodsReServer is run.
 */

int
getAndConnReHost (rsComm_t *rsComm, rodsServerHost_t **rodsServerHost)
{
    int status;

    status = getReHost (rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
	  "getAndConnReHost:getReHost() failed. erro=%d", status);
         return (status);
    }

    if ((*rodsServerHost)->localFlag == LOCAL_HOST) {
        return (LOCAL_HOST);
    }

    status = svrToSvrConnect (rsComm, *rodsServerHost);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "getAndConnReHost: svrToSvrConnect to %s failed",
          (*rodsServerHost)->hostName->name);
    }
    if (status >= 0) {
        return (REMOTE_HOST);
    } else {
        return (status);
    }
}

int
initConnectControl ()
{
    char *conFile;
    char *configDir;
    FILE *file;
    char buf[LONG_NAME_LEN * 5];
    int len;
    char *bufPtr;
    int status;
    struct allowedUser *tmpAllowedUser;
    int allowUserFlag = 0;
    int disallowUserFlag = 0;

    configDir = getConfigDir ();
    len = strlen (configDir) + strlen(CONNECT_CONTROL_FILE) + 2;
;

    conFile = (char *) malloc(len);

    snprintf (conFile, len, "%s/%s", configDir, CONNECT_CONTROL_FILE);
    file = fopen(conFile, "r");

    if (file == NULL) {
#ifdef DEBUG_CONNECT_CONTROL
            fprintf (stderr, "Unable to open CONNECT_CONTROL_FILE file %s\n",
             conFile);
#endif
        free (conFile);
        return (0);
    }

    free (conFile);

    MaxConnections = DEF_MAX_CONNECTION;	/* no limit */
    freeAllAllowedUser (AllowedUserHead);
    freeAllAllowedUser (DisallowedUserHead);
    AllowedUserHead = DisallowedUserHead = NULL;
    while (fgets (buf, LONG_NAME_LEN * 5, file) != NULL) {
        char myuser[NAME_LEN];
        char myZone[NAME_LEN];
        char myInput[NAME_LEN * 2];

        if (*buf == '#')        /* a comment */
            continue;

        bufPtr = buf;

        while (copyStrFromBuf (&bufPtr, myInput, NAME_LEN * 2) > 0) {
            if (strcmp (myInput, MAX_CONNECTIONS_KW) == 0) {
                if (copyStrFromBuf (&bufPtr, myInput, NAME_LEN) > 0) {
                    /* sanity check */
                    if (isdigit (*myInput)) {
                        MaxConnections = atoi (myInput);
                    } else {
                        rodsLog (LOG_ERROR,
                          "initConnectControl: inp maxConnections %d is not an int",
                       myInput);
                    }
                    break;
                }
            } else if (strcmp (myInput, ALLOWED_USER_LIST_KW) == 0) {
                if (disallowUserFlag == 0) {
                    allowUserFlag = 1;
                    break;
                } else {
                    rodsLog (LOG_ERROR,
                      "initConnectControl: both allowUserList and disallowUserList are set");
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
                }
            } else if (strcmp (myInput, DISALLOWED_USER_LIST_KW) == 0) {
                if (allowUserFlag == 0) {
                    disallowUserFlag = 1;
                    break;
                } else {
                    rodsLog (LOG_ERROR,
                      "initConnectControl: both allowUserList and disallowUserList are set");
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
                }
            }
            status = parseUserName (myInput, myuser, myZone);
            if (status >= 0) {
                if (strlen (myZone) == 0) {
                    zoneInfo_t *tmpZoneInfo;
                    if (getLocalZoneInfo (&tmpZoneInfo) >= 0) {
                        rstrcpy (myZone, tmpZoneInfo->zoneName, NAME_LEN);
                    }
                }
                tmpAllowedUser = (struct allowedUser*)
		  malloc (sizeof (struct allowedUser));
                memset (tmpAllowedUser, 0, sizeof (struct allowedUser));
		rstrcpy (tmpAllowedUser->userName, myuser, NAME_LEN);
		rstrcpy (tmpAllowedUser->rodsZone, myZone, NAME_LEN);
                /* queue it */

		if (allowUserFlag != 0) {
		    queAllowedUser (tmpAllowedUser, &AllowedUserHead);
		} else if (disallowUserFlag != 0) {
                    queAllowedUser (tmpAllowedUser, &DisallowedUserHead);
		} else {
                    rodsLog (LOG_ERROR,
                      "initConnectControl: neither allowUserList nor disallowUserList has been set");
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
		}
            } else {
                rodsLog (LOG_NOTICE,
                  "initConnectControl: cannot parse input %s. status = %d",
                  myInput, status);
            }
        }
    }

    fclose (file);
    return (0);
}

int
chkAllowedUser (char *userName, char *rodsZone)
{
    int status;

    if (userName == NULL || rodsZone == 0) {
        return (SYS_USER_NOT_ALLOWED_TO_CONN);
    }

    if (strlen (userName) == 0) {
        /* XXXXXXXXXX userName not yet defined. allow it for now */
        return 0;
    }

    if (AllowedUserHead != NULL) {
	status = matchAllowedUser (userName, rodsZone, AllowedUserHead);
	if (status == 1) {	/* a match */
	    return 0;
	} else {
            return SYS_USER_NOT_ALLOWED_TO_CONN;
	}
    } else if (DisallowedUserHead != NULL) {
        status = matchAllowedUser (userName, rodsZone, DisallowedUserHead);
	if (status == 1) {	/* a match, disallow */
	    return SYS_USER_NOT_ALLOWED_TO_CONN;
	} else {
	    return 0;
	}
    } else {
	/* no control, return 0 */
	return 0;
    }
}

int
matchAllowedUser (char *userName, char *rodsZone, 
struct allowedUser *allowedUserHead)
{
    struct allowedUser *tmpAllowedUser;

    if (allowedUserHead == NULL)
        return 0;

    tmpAllowedUser = allowedUserHead;
    while (tmpAllowedUser != NULL) {
        if (tmpAllowedUser->userName != NULL &&
         strcmp (tmpAllowedUser->userName, userName) == 0 &&
         tmpAllowedUser->rodsZone != NULL &&
         strcmp (tmpAllowedUser->rodsZone, rodsZone) == 0) {
            /* we have a match */
            break;
        }
        tmpAllowedUser = tmpAllowedUser->next;
    }
    if (tmpAllowedUser == NULL) {
        /* no match */
        return (0);
    } else {
        return (1);
    }
}

int
queAllowedUser (struct allowedUser *allowedUser, 
struct allowedUser **allowedUserHead)
{
    if (allowedUserHead == NULL || allowedUser == NULL) 
	return USER__NULL_INPUT_ERR;

    if (*allowedUserHead == NULL) {
        *allowedUserHead = allowedUser;
    } else {
        allowedUser->next = *allowedUserHead;
        *allowedUserHead = allowedUser;
    }
    return 0;
}

int
freeAllAllowedUser (struct allowedUser *allowedUserHead)
{
    struct allowedUser *tmpAllowedUser, *nextAllowedUser;
    tmpAllowedUser = allowedUserHead;
    while (tmpAllowedUser != NULL) {
        nextAllowedUser = tmpAllowedUser->next;
        free (tmpAllowedUser);
        tmpAllowedUser = nextAllowedUser;
    }
    return (0);
}

int
initAndClearProcLog ()
{
    initProcLog ();
    mkdir (ProcLogDir, DEFAULT_DIR_MODE);
    rmFilesInDir (ProcLogDir);

    return 0;
}

int
initProcLog ()
{
    snprintf (ProcLogDir, MAX_NAME_LEN, "%s/%s", 
      getLogDir(), PROC_LOG_DIR_NAME);
    return 0;
}

int
logAgentProc (rsComm_t *rsComm)
{
    FILE *fptr;
    char procPath[MAX_NAME_LEN];
    char *remoteAddr;
    char *progName;
    char *clientZone, *proxyZone;

    if (rsComm->procLogFlag == PROC_LOG_DONE) return 0;

    if (*rsComm->clientUser.userName == '\0' || 
      *rsComm->proxyUser.userName == '\0') 
        return 0;

    if (*rsComm->clientUser.rodsZone == '\0') {
        if ((clientZone = getLocalZoneName ()) == NULL) {
	    clientZone = "UNKNOWN";
	}
    } else {
	clientZone = rsComm->clientUser.rodsZone;
    }

    if (*rsComm->proxyUser.rodsZone == '\0') {
        if ((proxyZone = getLocalZoneName ()) == NULL) {
            proxyZone = "UNKNOWN";
        }
    } else {
        proxyZone = rsComm->proxyUser.rodsZone;
    }

    remoteAddr = inet_ntoa (rsComm->remoteAddr.sin_addr);
    if (remoteAddr == NULL || *remoteAddr == '\0') remoteAddr = "UNKNOWN";
    if (*rsComm->option == '\0')
        progName = "UNKNOWN";
    else
        progName = rsComm->option;

    snprintf (procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, getpid ());

    fptr = fopen (procPath, "w");

    if (fptr == NULL) {
        rodsLog (LOG_ERROR,
          "logAgentProc: Cannot open input file %s. ernro = %d",
          procPath, errno);
        return (UNIX_FILE_OPEN_ERR - errno);
    }

    fprintf (fptr, "%s %s %s %s %s %s %u\n",
      rsComm->proxyUser.userName, clientZone,
      rsComm->clientUser.userName, proxyZone,
      progName, remoteAddr, (unsigned int) time (0));

    rsComm->procLogFlag = PROC_LOG_DONE;
    fclose (fptr);
    return 0;
}

int
rmProcLog (int pid)
{
    char procPath[MAX_NAME_LEN];

    snprintf (procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, pid);
    unlink (procPath);
    return 0;
}

int
readProcLog (int pid, procLog_t *procLog)
{
    FILE *fptr;
    char procPath[MAX_NAME_LEN];
    int status;

    if (procLog == NULL) return USER__NULL_INPUT_ERR;

    snprintf (procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, pid);

    fptr = fopen (procPath, "r");

    if (fptr == NULL) {
        rodsLog (LOG_ERROR,
          "readProcLog: Cannot open input file %s. ernro = %d",
          procPath, errno);
        return (UNIX_FILE_OPEN_ERR - errno);
    }

    procLog->pid = pid;

    status = fscanf (fptr, "%s %s %s %s %s %s %u",
      procLog->clientName, procLog->clientZone,
      procLog->proxyName, procLog->proxyZone,
      procLog->progName, procLog->remoteAddr, &procLog->startTime);

    if (status == 7) {	/* 7 parameters */
	status = 0;
    } else {
        rodsLog (LOG_ERROR,
          "readProcLog: error fscanf file %s. Number of param read = %d",
          procPath, status);
	status = UNIX_FILE_READ_ERR;
    }
    fclose (fptr);
    return status;
}

int
setRsCommFromRodsEnv (rsComm_t *rsComm)
{
    rodsEnv *myEnv = &rsComm->myEnv;

    rstrcpy (rsComm->proxyUser.userName, myEnv->rodsUserName, NAME_LEN);
    rstrcpy (rsComm->clientUser.userName, myEnv->rodsUserName, NAME_LEN);

    rstrcpy (rsComm->proxyUser.rodsZone, myEnv->rodsZone, NAME_LEN);
    rstrcpy (rsComm->clientUser.rodsZone, myEnv->rodsZone, NAME_LEN);

    return (0);
}

int
queAgentProc (agentProc_t *agentProc, agentProc_t **agentProcHead,
irodsPosition_t position)
{
    if (agentProc == NULL || agentProcHead == NULL) return USER__NULL_INPUT_ERR;

    if (*agentProcHead == NULL) {
        *agentProcHead = agentProc;
        agentProc->next = NULL;
        return 0;
    }

    if (position == TOP_POS) {
        agentProc->next = *agentProcHead;
        *agentProcHead = agentProc;
    } else {
        agentProc_t *tmpAgentProc = *agentProcHead;
        while (tmpAgentProc->next != NULL) {
            tmpAgentProc = tmpAgentProc->next;
        }
        tmpAgentProc->next = agentProc;
        agentProc->next = NULL;
    }
    return 0;
}

