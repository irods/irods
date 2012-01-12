/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.h - common header file for initServer.c
 */



#ifndef INIT_SERVER_H
#define INIT_SERVER_H

#include "rods.h"
#include "sockComm.h"
#include "rsLog.h"

/* server host configuration */

#define RCAT_HOST_FILE  "server.config"
#ifndef windows_platform
#define HOST_CONFIG_FILE  "irodsHost"
#define RE_RULES_FILE   "reRules"
#else
#define HOST_CONFIG_FILE  "irodsHost.txt"
#define RE_RULES_FILE   "reRules.txt"
#endif
#define CONNECT_CONTROL_FILE	"connectControl.config"
/* Keywords used in CONNECT_CONTROL_FILE */
#define MAX_CONNECTIONS_KW		"maxConnections"
#define ALLOWED_USER_LIST_KW		"allowUserList"
#define DISALLOWED_USER_LIST_KW	"disallowUserList"
#define NO_MAX_CONNECTION_LIMIT	-1
#define DEF_MAX_CONNECTION	NO_MAX_CONNECTION_LIMIT		 

/* keywords for the RCAT_HOST_FILE */
#define ICAT_HOST_KW		"icatHost"
#define SLAVE_ICAT_HOST_KW	"slaveIcatHost"

#define RE_HOST_KW		"reHost"
#define XMSG_HOST_KW		"xmsgHost"

/* Keywords for the RULE ENGINE initialization */
#define RE_RULESET_KW           "reRuleSet"
#define RE_FUNCMAPSET_KW        "reFuncMapSet"
#define RE_VARIABLEMAPSET_KW    "reVariableMapSet"

/* Keywords for Kerberos initialization */
#define KERBEROS_NAME_KW "KerberosName"


/* definition for initialization state InitialState and IcatConnState */

#define INITIAL_NOT_DONE	0
#define INITIAL_DONE		1

/* rsPipSigalHandler parameters */
#define MAX_BROKEN_PIPE_CNT	50
#define BROKEN_PIPE_INT		300	/* 5 minutes interval */

/* Managing the spawned agents */

typedef struct agentProc {
    int pid;
    int sock;
    startupPack_t startupPack;
    struct sockaddr_in  remoteAddr;  /* remote address */
    struct agentProc *next;
} agentProc_t;

typedef struct hostName {
    char *name;
    struct hostName *next;
} hostName_t;

/* definition for localFlag */
#define UNKNOWN_HOST_LOC -1
#define LOCAL_HOST	0
#define REMOTE_HOST	1
#define REMOTE_GW_HOST  2	/* remote gateway host */
#define REMOTE_ZONE_HOST  3     /* host in remote zone */

/* definition for rcatEnabled */

#define NOT_RCAT_ENABLED	0
#define LOCAL_ICAT		1
#define LOCAL_SLAVE_ICAT	2
#define REMOTE_ICAT		3

/* definition for runMode */

#define SINGLE_PASS             0
#define IRODS_SERVER            1
#define STANDALONE_SERVER       2

typedef struct rodsServerHost {
    hostName_t *hostName;
    rcComm_t *conn;
    int rcatEnabled;
    int reHostFlag;
    int xmsgHostFlag;
    int localFlag;
    int status;
    void *zoneInfo;
    struct rodsServerHost *next;
} rodsServerHost_t;

typedef struct zoneInfo {
    char zoneName[NAME_LEN];
    int portNum;
    rodsServerHost_t *masterServerHost;
    rodsServerHost_t *slaveServerHost;
    struct zoneInfo *next;
} zoneInfo_t;

/* definitions for Server ID information */
#define MAX_FED_RSIDS  5
#define LOCAL_ZONE_SID_KW       "LocalZoneSID"
#define REMOTE_ZONE_SID_KW      "RemoteZoneSID"
#define SID_KEY_KW              "SIDKey"

struct allowedUser {
    char userName[NAME_LEN];
    char rodsZone[NAME_LEN];
    struct allowedUser *next;
};

int
resolveHost (rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost);
int
resolveHostByDataObjInfo (dataObjInfo_t *dataObjInfo,
rodsServerHost_t **rodsServerHost);
int
resolveHostByRescInfo (rescInfo_t *rescInfo, rodsServerHost_t **rodsServerHost);
int
initServerInfo (rsComm_t *rsComm);
int
initLocalServerHost (rsComm_t *rsComm);
int
initRcatServerHostByFile (rsComm_t *rsComm);
int
queAddr (rodsServerHost_t *rodsServerHost, char *myHostName);
int
queHostName (rodsServerHost_t *rodsServerHost, char *myHostName, int topFlag);
int
queRodsServerHost (rodsServerHost_t **rodsServerHostHead,
rodsServerHost_t *myRodsServerHost);
int
getAndConnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost);
char *
getConfigDir();
char *
getLogDir();
rodsServerHost_t *
mkServerHost (char *myHostAddr, char *zoneName);
int
getRcatHost (int rcatType, char *rcatZoneHint,  
rodsServerHost_t **rodsServerHost);
int
initZone (rsComm_t *rsComm);
int
queZone (char *zoneName, int portNum, rodsServerHost_t *masterServerHost,
rodsServerHost_t *slaveServerHost);
int
printZoneInfo ();
int
printServerHost (rodsServerHost_t *myServerHost);
int
printZoneInfo ();
int
getAndConnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost);
int
getAndConnRcatHostNoLogin (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost);
int
getAndDisconnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
rodsServerHost_t **rodsServerHost);
int
setExecArg (char *commandArgv, char *av[]);
#ifdef RULE_ENGINE_N
int
initAgent (int processType, rsComm_t *rsComm);
#else
int
initAgent (rsComm_t *rsComm);
#endif
void cleanupAndExit (int status);
#ifdef  __cplusplus
void signalExit ( int );
void
rsPipSigalHandler ( int );
#else
void signalExit ();
void
rsPipSigalHandler ();
#endif

int
initHostConfigByFile (rsComm_t *rsComm);
int
matchHostConfig (rodsServerHost_t *myRodsServerHost);
int
queConfigName (rodsServerHost_t *configServerHost,
rodsServerHost_t *myRodsServerHost);
int
disconnectAllSvrToSvrConn ();
int
disconnRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint);
int
svrReconnect (rsComm_t *rsComm);
int
initRsComm (rsComm_t *rsComm);
void
daemonize (int runMode, int logFd);
int
logFileOpen (int runMode, char *logDir, char *logFileName);
int
initRsCommWithStartupPack (rsComm_t *rsComm, startupPack_t *startupPack);
int
getLocalZoneInfo (zoneInfo_t **outZoneInfo);
char *
getLocalZoneName ();
int
getZoneInfo (char *rcatZoneHint, zoneInfo_t **myZoneInfo);
int
getAndConnRemoteZone (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsServerHost_t **rodsServerHost, char *remotZoneOpr);
int
getRemoteZoneHost (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rodsServerHost_t **rodsServerHost, char *remotZoneOpr);
int
isLocalZone (char *zoneHint);
int
isSameZone (char *zoneHint1, char *zoneHint2);
int
convZoneSockError (int inStatus);
int
resetRcatHost (rsComm_t *rsComm, int rcatType, char *rcatZoneHint);
int
getReHost (rodsServerHost_t **rodsServerHost);
int
getAndConnReHost (rsComm_t *rsComm, rodsServerHost_t **rodsServerHost);
int
isLocalHost (char *hostAddr);
int
initConnectControl ();
int
chkAllowedUser (char *userName, char *rodsZone);
int
queAllowedUser (struct allowedUser *allowedUser,
struct allowedUser **allowedUserHead);
int
matchAllowedUser (char *userName, char *rodsZone,
struct allowedUser *allowedUserHead);
int
freeAllAllowedUser (struct allowedUser *allowedUserHead);
int
initAndClearProcLog ();
int
initProcLog ();
int
logAgentProc ( rsComm_t* );
int
readProcLog (int pid, procLog_t *procLog);
int
rmProcLog (int pid);
int
getXmsgHost (rodsServerHost_t **rodsServerHost);
int
setRsCommFromRodsEnv (rsComm_t *rsComm);
int
queAgentProc (agentProc_t *agentPorc, agentProc_t **agentPorcHead,
irodsPosition_t position);
#endif	/* INIT_SERVER_H */
