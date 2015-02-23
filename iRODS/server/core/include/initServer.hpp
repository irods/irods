/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.h - common header file for initServer.c
 */



#ifndef INIT_SERVER_HPP
#define INIT_SERVER_HPP

#include "rods.hpp"
//#include "sockComm.hpp"
#include "rsLog.hpp"

/* server host configuration */

#ifndef windows_platform
#define HOST_CONFIG_FILE  "hosts_config.json"
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

/* keywords for the server config */
#define ICAT_HOST_KW		"icatHost"
#define SLAVE_ICAT_HOST_KW	"slaveIcatHost"

#define RE_HOST_KW		"reHost"
#define XMSG_HOST_KW		"xmsgHost"

/* Keywords for the RULE ENGINE initialization */
#define RE_RULESET_KW           "reRuleSet"
#define RE_FUNCMAPSET_KW        "reFuncMapSet"
#define RE_VARIABLEMAPSET_KW    "reVariableMapSet"
#define RE_CACHE_SALT_KW    "reCacheSalt"

/* Keywords for Kerberos initialization */
#define KERBEROS_NAME_KW "KerberosServicePrincipal"
#define KERBEROS_KEYTAB_KW "KerberosKeytab"

/* definition for initialization state InitialState and IcatConnState */

#define INITIAL_NOT_DONE	0
#define INITIAL_DONE		1

/* rsPipSigalHandler parameters */
#define MAX_BROKEN_PIPE_CNT	50
#define BROKEN_PIPE_INT		300	/* 5 minutes interval */

#define LOCK_FILE_PURGE_TIME   7200    /* purge lock files every 2 hr. */

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
#define SERVER            1
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
resolveHost( rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost );
int
resolveHostByDataObjInfo( dataObjInfo_t *dataObjInfo,
                          rodsServerHost_t **rodsServerHost );
int
resoAndConnHostByDataObjInfo( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                              rodsServerHost_t **rodsServerHost );
int
initServerInfo( rsComm_t *rsComm );
int
initLocalServerHost();
int
initRcatServerHostByFile();
int
queAddr( rodsServerHost_t *rodsServerHost, char *myHostName );
int
queHostName( rodsServerHost_t *rodsServerHost, const char *myHostName, int topFlag );
int
queRodsServerHost( rodsServerHost_t **rodsServerHostHead,
                   rodsServerHost_t *myRodsServerHost );
int
getAndConnRcatHost( rsComm_t *rsComm, int rcatType, const char *rcatZoneHint,
                    rodsServerHost_t **rodsServerHost );
char *
getConfigDir();
char *
getLogDir();
rodsServerHost_t *
mkServerHost( char *myHostAddr, char *zoneName );
int
getRcatHost( int rcatType, const char *rcatZoneHint,
             rodsServerHost_t **rodsServerHost );
int
initZone( rsComm_t *rsComm );
int
queZone( const char *zoneName, int portNum, rodsServerHost_t *masterServerHost,
         rodsServerHost_t *slaveServerHost );
int
printZoneInfo();
int
printServerHost( rodsServerHost_t *myServerHost );
int
printZoneInfo();
int
getAndConnRcatHostNoLogin( rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
                           rodsServerHost_t **rodsServerHost );
int
getAndDisconnRcatHost( int rcatType, char *rcatZoneHint,
                       rodsServerHost_t **rodsServerHost );
int
setExecArg( char *commandArgv, char *av[] );
int
initAgent( int processType, rsComm_t *rsComm );
void cleanup();
void cleanupAndExit( int status );
void signalExit( int );
void
rsPipSigalHandler( int );

int
initHostConfigByFile();
int
matchHostConfig( rodsServerHost_t *myRodsServerHost );
int
queConfigName( rodsServerHost_t *configServerHost,
               rodsServerHost_t *myRodsServerHost );
int
disconnectAllSvrToSvrConn();
int
disconnRcatHost( int rcatType, const char *rcatZoneHint );
int
svrReconnect( rsComm_t *rsComm );
int
initRsComm( rsComm_t *rsComm );
void
daemonize( int runMode, int logFd );
int
logFileOpen( int runMode, char *logDir, char *logFileName );
int
initRsCommWithStartupPack( rsComm_t *rsComm, startupPack_t *startupPack );
int
getLocalZoneInfo( zoneInfo_t **outZoneInfo );
char *
getLocalZoneName();
int
getZoneInfo( const char *rcatZoneHint, zoneInfo_t **myZoneInfo );
int
getAndConnRemoteZone( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                      rodsServerHost_t **rodsServerHost, char *remotZoneOpr );
int
getRemoteZoneHost( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   rodsServerHost_t **rodsServerHost, char *remotZoneOpr );
int
isLocalZone( char *zoneHint );
int
isSameZone( char *zoneHint1, char *zoneHint2 );
int
convZoneSockError( int inStatus );
int
resetRcatHost( int rcatType, const char *rcatZoneHint );
int
getReHost( rodsServerHost_t **rodsServerHost );
int
getAndConnReHost( rsComm_t *rsComm, rodsServerHost_t **rodsServerHost );
int
isLocalHost( const char *hostAddr );
int
initConnectControl();
int
chkAllowedUser( char *userName, char *rodsZone );
int
queAllowedUser( struct allowedUser *allowedUser,
                struct allowedUser **allowedUserHead );
int
matchAllowedUser( char *userName, char *rodsZone,
                  struct allowedUser *allowedUserHead );
int
freeAllAllowedUser( struct allowedUser *allowedUserHead );
int
initAndClearProcLog();
int
initProcLog();
int
logAgentProc( rsComm_t* );
int
readProcLog( int pid, procLog_t *procLog );
int
rmProcLog( int pid );
int
getXmsgHost( rodsServerHost_t **rodsServerHost );
int
setRsCommFromRodsEnv( rsComm_t *rsComm );
int
queAgentProc( agentProc_t *agentPorc, agentProc_t **agentPorcHead,
              irodsPosition_t position );
int
purgeLockFileDir( int chkLockFlag ); // JMC - backport 4612

#endif	/* INIT_SERVER_H */
