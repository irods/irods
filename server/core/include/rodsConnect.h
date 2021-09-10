#ifndef RODS_CONNECT_H
#define RODS_CONNECT_H

/// \file

#include "rodsDef.h"
#include "rcConnect.h"

#define HOST_CONFIG_FILE                "hosts_config.json"
#define RE_RULES_FILE                   "reRules"

// Keywords used in CONNECT_CONTROL_FILE
#define USER_WHITELIST_KW               "whitelist"
#define USER_BLACKLIST_KW               "blacklist"
#define NO_MAX_CONNECTION_LIMIT         -1
#define DEF_MAX_CONNECTION              NO_MAX_CONNECTION_LIMIT

// definition for initialization state InitialState and IcatConnState
#define INITIAL_NOT_DONE                0
#define INITIAL_DONE                    1

// rsPipeSignalHandler parameters
#define MAX_BROKEN_PIPE_CNT             50
#define BROKEN_PIPE_INT                 300  // 5 minutes interval

#define LOCK_FILE_PURGE_TIME            7200 // purge lock files every 2 hr.

// Managing the spawned agents
typedef struct agentProc {
    int pid;
    int sock;
    startupPack_t startupPack;
    struct sockaddr_in  remoteAddr;  // remote address
    struct agentProc *next;
} agentProc_t;

typedef struct hostName {
    char *name;
    struct hostName *next;
} hostName_t;

// definition for localFlag
#define UNKNOWN_HOST_LOC                -1
#define LOCAL_HOST                      0
#define REMOTE_HOST                     1
#define REMOTE_GW_HOST                  2 // remote gateway host
#define REMOTE_ZONE_HOST                3 // host in remote zone

// definition for rcatEnabled
#define NOT_RCAT_ENABLED                0
#define LOCAL_ICAT                      1
#define LOCAL_SLAVE_ICAT                2
#define REMOTE_ICAT                     3

// definition for runMode
#define SINGLE_PASS                     0
#define SERVER                          1
#define STANDALONE_SERVER               2

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

// definitions for Server ID information
#define MAX_FED_RSIDS                   5
#define LOCAL_ZONE_SID_KW               "LocalZoneSID"
#define REMOTE_ZONE_SID_KW              "RemoteZoneSID"
#define SID_KEY_KW                      "SIDKey"

#ifdef __cplusplus
extern "C" {
#endif

int queAddr(rodsServerHost_t *rodsServerHost, char *myHostName);

int queHostName(rodsServerHost_t *rodsServerHost,
                const char *myHostName,
                int topFlag);

int queRodsServerHost(rodsServerHost_t **rodsServerHostHead,
                      rodsServerHost_t *myRodsServerHost);

rodsServerHost_t* mkServerHost(char *myHostAddr, char *zoneName);

int queZone(const char *zoneName,
            int portNum,
            rodsServerHost_t *masterServerHost,
            rodsServerHost_t *slaveServerHost);

int matchHostConfig(rodsServerHost_t *myRodsServerHost);

int queConfigName(rodsServerHost_t *configServerHost,
                  rodsServerHost_t *myRodsServerHost);

int getAndConnRcatHost(rsComm_t *rsComm,
                       int rcatType,
                       const char *rcatZoneHint,
                       rodsServerHost_t **rodsServerHost);

int getAndConnRcatHostNoLogin(rsComm_t *rsComm,
                              int rcatType,
                              char *rcatZoneHint,
                              rodsServerHost_t **rodsServerHost);

/// \brief Gets the Catalog Service Provider host information from `ZoneInfoHead`.
///
/// \parblock
/// The information populated in `*rodsServerHost` will be a pointer to `masterServerHost` or
/// `slaveServerHost` of the global `ZoneInfoHead` list. These linked lists are identical to the
/// pointers in the global linked list of server information called `ServerHostHead`.
///
/// This function should be used for getting this pointer in particular because any connections
/// made to the retrieved server will be reused from previous redirections, if applicable, and
/// cleaned up automatically on agent teardown.
/// \endparblock
int getRcatHost(int rcatType,
                const char *rcatZoneHint,
                rodsServerHost_t **rodsServerHost);

int disconnRcatHost(int rcatType, const char *rcatZoneHint);

/// \brief Gets the Catalog Service Provider host from `ZoneInfoHead` and connects to it.
///
/// \parblock
/// The information populated in `*rodsServerHost` will be a pointer to `masterServerHost` or
/// `slaveServerHost` of the global `ZoneInfoHead` list. These linked lists are identical to the
/// pointers in the global linked list of server information called `ServerHostHead`.
///
/// This function should be used for making any redirect connections to the Catalog Service
/// Provider. Connections will be reused from previous redirections, if applicable, and cleaned
/// up automatically on agent teardown.
/// \endparblock
int getAndDisconnRcatHost(int rcatType,
                          char *rcatZoneHint,
                          rodsServerHost_t **rodsServerHost);

/// \brief Disconnects and cleans up all server-to-server connections made by this agent.
///
/// \parblock
/// This function walks the list of connected iRODS servers in `ServerHostHead` and disconnects
/// them. This is convenient at agent teardown to ensure that the agent is not leaving any open
/// connections or leaking memory from the connection structures.
/// \endparblock
int disconnectAllSvrToSvrConn();

int svrReconnect(rsComm_t *rsComm);

int getAndConnRemoteZone(rsComm_t *rsComm,
                         dataObjInp_t *dataObjInp,
                         rodsServerHost_t **rodsServerHost,
                         char *remotZoneOpr);

int getReHost(rodsServerHost_t **rodsServerHost);

int getAndConnReHost(rsComm_t *rsComm, rodsServerHost_t **rodsServerHost);

int resetRcatHost(int rcatType, const char *rcatZoneHint);

int isLocalHost(const char *hostAddr);

int getXmsgHost(rodsServerHost_t **rodsServerHost);

int getLocalZoneInfo(zoneInfo_t **outZoneInfo);

char* getLocalZoneName();

int getZoneInfo(const char *rcatZoneHint, zoneInfo_t **myZoneInfo);

int getRemoteZoneHost(rsComm_t *rsComm,
                      dataObjInp_t *dataObjInp,
                      rodsServerHost_t **rodsServerHost,
                      char *remotZoneOpr);

int isLocalZone(const char *zoneHint);

int isSameZone(char *zoneHint1, char *zoneHint2);

int convZoneSockError(int inStatus);

int resolveHost(rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost);

int resolveHostByDataObjInfo(dataObjInfo_t *dataObjInfo,
                             rodsServerHost_t **rodsServerHost);

int resoAndConnHostByDataObjInfo(rsComm_t *rsComm,
                                 dataObjInfo_t *dataObjInfo,
                                 rodsServerHost_t **rodsServerHost);

int printZoneInfo();

int printServerHost(rodsServerHost_t *myServerHost);

int printZoneInfo();

#ifdef __cplusplus
} //extern C
#endif

#endif // RODS_CONNECT_H

