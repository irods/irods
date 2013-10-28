/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rcConnect.h - common header file for client connect
 */



#ifndef RC_CONNECT_H
#define RC_CONNECT_H

#include "rodsDef.h"
#include "rodsVersion.h"
#include "rodsError.h"
#include "rodsLog.h"
#include "stringOpr.h"
#include "rodsType.h"
#include "rodsUser.h"
#include "getRodsEnv.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "irodsGuiProgressCallback.h"


#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

// =-=-=-=-=-=-=-
// ssl includes
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    PROCESSING_STATE,	 /* the process is not sending nor receiving */
    RECEIVING_STATE,
    SENDING_STATE,
    CONN_WAIT_STATE
} procState_t;

typedef struct reconnMsg {
    int status;
    int cookie;
    procState_t procState;
    int flag;
} reconnMsg_t;

/* one seq per thread */
typedef struct dataSeg {
    rodsLong_t len;
    rodsLong_t offset;
} dataSeg_t;

typedef enum {
    FILE_RESTART_OFF,
    FILE_RESTART_ON
} fileRestartFlag_t;

typedef enum {
    FILE_NOT_RESTART,
    FILE_RESTARTED
} fileRestartStatus_t;

typedef struct {
    char fileName[MAX_NAME_LEN];        /* the local file name to restart */
    char objPath[MAX_NAME_LEN];         /* the irodsPath */
    int numSeg;         /* number of segments. should equal to num threads */
    fileRestartStatus_t status;         /* restart status  */
    rodsLong_t fileSize;
    dataSeg_t dataSeg[MAX_NUM_CONFIG_TRAN_THR];
} fileRestartInfo_t;

typedef struct {
    fileRestartFlag_t flags;
    rodsLong_t writtenSinceUpdated;	/* bytes trans since last update */
    char infoFile[MAX_NAME_LEN];        /* file containing restart info */
    fileRestartInfo_t info;     /* must be the last item because of PI */
} fileRestart_t;

typedef enum {
    PROC_LOG_NOT_DONE,  /* the proc logging in log/proc not done yet */
    PROC_LOG_DONE       /* the proc logging in log/proc is done */
} procLogFlag_t;

/* The client connection handle */

typedef struct {
    irodsProt_t                irodsProt;
    char                       host[NAME_LEN];
    int                        sock;
    int                        portNum;
    int                        loggedIn;	/* already logged in ? */
    struct sockaddr_in         localAddr;   /* local address */
    struct sockaddr_in         remoteAddr;  /* remote address */
    userInfo_t                 proxyUser;
    userInfo_t                 clientUser;
    version_t*                 svrVersion;	/* the server's version */
    rError_t*                  rError;
    int                        flag;
    transferStat_t             transStat;
    int                        apiInx;
    int                        status;
    int                        windowSize;
    int                        reconnectedSock;
    time_t                     reconnTime;
    volatile bool		       exit_flg;
    boost::thread*             reconnThr;
    boost::mutex*              lock;
    boost::condition_variable* cond;
    procState_t                agentState;
    procState_t                clientState;
    procState_t                reconnThrState;
    operProgress_t             operProgress;
    
    int  key_size;
    int  salt_size;
    int  num_hash_rounds;
    char encryption_algorithm[ NAME_LEN ];
    char negotiation_results[ MAX_NAME_LEN ];
    char shared_secret      [ NAME_LEN ];

    int                        ssl_on;
    SSL_CTX*                   ssl_ctx;
    SSL*                       ssl;
    
    // =-=-=-=-=-=-=-
    // this struct needs to stay at the bottom of
    // rcComm_t
    fileRestart_t              fileRestart;

} rcComm_t;

typedef struct {
    int orphanCnt;
    int nonOrphanCnt;
} perfStat_t;

/* the server connection handle. probably should go somewhere else */
typedef struct {
    irodsProt_t irodsProt;
    int sock;
    int connectCnt;
    struct sockaddr_in  localAddr;   /* local address */
    struct sockaddr_in  remoteAddr;  /* remote address */
    char clientAddr[NAME_LEN]; 	/* str version of remoteAddr */
    userInfo_t proxyUser;
    userInfo_t clientUser;
    rodsEnv myEnv;	/* the local user */
    version_t cliVersion;      /* the client's version */
    char option[NAME_LEN];
    procLogFlag_t procLogFlag;
    rError_t rError;
    portalOpr_t *portalOpr;
    int apiInx;
    int status;
    perfStat_t perfStat;
    int windowSize;
    int reconnFlag;
    int reconnSock;
    int reconnPort;
    int reconnectedSock;
    char *reconnAddr;
    int cookie;

    boost::thread*              reconnThr;
    boost::mutex*               lock;
    boost::condition_variable*  cond;
    procState_t agentState;	
    procState_t clientState;
    procState_t reconnThrState;
    int gsiRequest;

#ifdef USE_SSL
    int ssl_on;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    int ssl_do_accept;
    int ssl_do_shutdown;
#endif
    
    char negotiation_results[ MAX_NAME_LEN ];
    char shared_secret      [ NAME_LEN ];

    int  key_size;
    int  salt_size;
    int  num_hash_rounds;
    char encryption_algorithm[ NAME_LEN ];

} rsComm_t;

void rcPipSigHandler ();

rcComm_t *
rcConnect (char *rodsHost, int rodsPort, char *userName, char *rodsZone,
int reconnFlag, rErrMsg_t *errMsg);

rcComm_t *
_rcConnect (char *rodsHost, int rodsPort,
char *proxyUserName, char *proxyRodsZone,
char *clientUserName, char *clientRodsZone, rErrMsg_t *errMsg, int connectCnt,
int reconnFlag);

int
setUserInfo (
char *proxyUserName, char *proxyRodsZone,
char *clientUserName, char *clientRodsZone,
userInfo_t *clientUser, userInfo_t *proxyUser);

int
setRhostInfo (rcComm_t *conn, char *rodsHost, int rodsPort);
int 
setSockAddr (struct sockaddr_in *remoteAddr, char *rodsHost, int rodsPort);

int setAuthInfo (char *rodsAuthScheme,
char *authStr, char *rodsServerDn,
userInfo_t *clientUser, userInfo_t *proxyUser, int flag);

int
rcDisconnect (rcComm_t *conn);
int
freeRcComm (rcComm_t *conn);
int
cleanRcComm (rcComm_t *conn);
/* XXXX putting clientLogin here for now. Should be in clientLogin.h */
int
clientLogin(rcComm_t *conn, const char* _context = 0, const char* _scheme_override = 0 );
int
clientLoginPam(rcComm_t *conn, char *password, int ttl);

char *
getSessionSignitureClientside();

int
clientLoginWithPassword(rcComm_t *conn, char* password);
rcComm_t *
rcConnectXmsg (rodsEnv *myRodsEnv, rErrMsg_t *errMsg);
void
cliReconnManager (rcComm_t *conn);
int
cliChkReconnAtSendStart (rcComm_t *conn);
int
cliChkReconnAtSendEnd (rcComm_t *conn);
int
cliChkReconnAtReadStart (rcComm_t *conn);
int
cliChkReconnAtReadEnd (rcComm_t *conn);

#ifdef  __cplusplus
}
#endif

#endif	/* RC_CONNECT_H */
