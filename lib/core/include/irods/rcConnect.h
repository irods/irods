#ifndef IRODS_RC_CONNECT_H
#define IRODS_RC_CONNECT_H

#include "irods/rodsDef.h"
#include "irods/rodsError.h"
#include "irods/rodsLog.h"
#include "irods/stringOpr.h"
#include "irods/rodsType.h"
#include "irods/rodsUser.h"
#include "irods/getRodsEnv.h"
#include "irods/objInfo.h"
#include "irods/dataObjInpOut.h"
#include "irods/guiProgressCallback.h"

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

/* definition for the reconnFlag */
#define NO_RECONN       0       /* no reconnection */
#define RECONN_NOTUSED  1       /* this has been deprecated */
#define RECONN_TIMEOUT  200

#define RECONN_TIMEOUT_TIME  600   /* re-connection timeout time in sec */

// Forward declaration of thread context.
struct thread_context;

typedef enum ProcState {
    PROCESSING_STATE,    /* the process is not sending nor receiving */
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

typedef enum FileRestartFlag {
    FILE_RESTART_OFF,
    FILE_RESTART_ON
} fileRestartFlag_t;

typedef enum FileRestartStatus {
    FILE_NOT_RESTART,
    FILE_RESTARTED
} fileRestartStatus_t;

typedef struct FileRestartInfo {
    char fileName[MAX_NAME_LEN];        /* the local file name to restart */
    char objPath[MAX_NAME_LEN];         /* the irodsPath */
    int numSeg;         /* number of segments. should equal to num threads */
    fileRestartStatus_t status;         /* restart status  */
    rodsLong_t fileSize;
    dataSeg_t dataSeg[MAX_NUM_CONFIG_TRAN_THR];
} fileRestartInfo_t;

typedef struct FileRestart {
    fileRestartFlag_t flags;
    rodsLong_t writtenSinceUpdated;     /* bytes trans since last update */
    char infoFile[MAX_NAME_LEN];        /* file containing restart info */
    fileRestartInfo_t info;     /* must be the last item because of PI */
} fileRestart_t;

typedef enum ProcLogFlag {
    PROC_LOG_NOT_DONE,  /* the proc logging in log/proc not done yet */
    PROC_LOG_DONE       /* the proc logging in log/proc is done */
} procLogFlag_t;

/* The client connection handle */

typedef struct RcComm {
    irodsProt_t                irodsProt;
    char                       host[NAME_LEN];
    int                        sock;
    int                        portNum;
    int                        loggedIn;        /* already logged in ? */
    struct sockaddr_in         localAddr;   /* local address */
    struct sockaddr_in         remoteAddr;  /* remote address */
    userInfo_t                 proxyUser;
    userInfo_t                 clientUser;
    version_t*                 svrVersion;      /* the server's version */
    rError_t*                  rError;
    int                        flag;
    transferStat_t             transStat;
    int                        apiInx;
    int                        status;
    int                        windowSize;
    int                        reconnectedSock;
    time_t                     reconnTime;
    volatile int                       exit_flg;
    struct thread_context*     thread_ctx;
    procState_t                agentState;
    procState_t                clientState;
    procState_t                reconnThrState;
    operProgress_t             operProgress;

    int  key_size;
    int  salt_size;
    int  num_hash_rounds;
    char encryption_algorithm[ NAME_LEN ];
    char negotiation_results[ MAX_NAME_LEN ];
    unsigned char shared_secret[ NAME_LEN ];

    int                        ssl_on;
    SSL_CTX*                   ssl_ctx;
    SSL*                       ssl;

    // =-=-=-=-=-=-=-
    // this struct needs to stay at the bottom of
    // rcComm_t
    fileRestart_t              fileRestart;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    char session_signature[33]; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
} rcComm_t;

typedef struct PerfStat {
    int orphanCnt;
    int nonOrphanCnt;
} perfStat_t;

/* the server connection handle. probably should go somewhere else */
typedef struct RsComm {
    irodsProt_t irodsProt;
    int sock;
    int connectCnt;
    struct sockaddr_in localAddr;           // local address
    struct sockaddr_in remoteAddr;          // remote address
    char clientAddr[NAME_LEN];              // str version of remoteAddr
    userInfo_t proxyUser;
    userInfo_t clientUser;
    rodsEnv myEnv;                          // the local user
    version_t cliVersion;                   // the client's version
    char option[LONG_NAME_LEN];
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

    struct thread_context* thread_ctx;

    procState_t agentState;
    procState_t clientState;
    procState_t reconnThrState;
    int gsiRequest;
    char* auth_scheme;

    int ssl_on;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    int ssl_do_accept;
    int ssl_do_shutdown;

    char negotiation_results[MAX_NAME_LEN];
    unsigned char shared_secret[NAME_LEN];

    int  key_size;
    int  salt_size;
    int  num_hash_rounds;
    char encryption_algorithm[NAME_LEN];

    // A key-value container that is available for general purpose
    // use throughout server-side operations.
    keyValPair_t session_props;
} rsComm_t;

#ifdef __cplusplus
extern "C" {
#endif

rcComm_t *
rcConnect( const char *rodsHost, int rodsPort, const char *userName, const char *rodsZone,
           int reconnFlag, rErrMsg_t *errMsg );

rcComm_t *
_rcConnect( const char *rodsHost, int rodsPort,
            const char *proxyUserName, const char *proxyRodsZone,
            const char *clientUserName, const char *clientRodsZone, rErrMsg_t *errMsg, int connectCnt,
            int reconnFlag );

int
setUserInfo(
    const char *proxyUserName, const char *proxyRodsZone,
    const char *clientUserName, const char *clientRodsZone,
    userInfo_t *clientUser, userInfo_t *proxyUser );

int
setRhostInfo( rcComm_t *conn, const char *rodsHost, int rodsPort );
int
setSockAddr( struct sockaddr_in *remoteAddr, const char *rodsHost, int rodsPort );

int setAuthInfo( char *rodsAuthScheme,
                 char *authStr, char *rodsServerDn,
                 userInfo_t *clientUser, userInfo_t *proxyUser, int flag );

int
rcDisconnect( rcComm_t *conn );
int
freeRcComm( rcComm_t *conn );
int
cleanRcComm( rcComm_t *conn );
/* XXXX putting clientLogin here for now. Should be in clientLogin.h */
#ifdef __cplusplus
int
clientLogin( rcComm_t *conn, const char* _context = 0, const char* _scheme_override = 0 );
#else
int
clientLogin( rcComm_t *conn, const char* _context, const char* _scheme_override );
#endif
int
clientLoginPam( rcComm_t *conn, char *password, int ttl );
int
clientLoginTTL( rcComm_t *conn, int ttl );

// clang-format off
__attribute__((deprecated("Use session_signature member variable in RcComm instance.")))
char* getSessionSignatureClientside();
// clang-format on

/// Stores the first 16 bytes of a buffer as hexadecimal in the \p session_signature member
/// variable of an RcComm.
///
/// If either pointer is NULL or the size of the buffer is too small, an error is returned.
///
/// \param[in] _comm        A pointer to a RcComm.
/// \param[in] _buffer      A buffer holding at least 16 bytes.
/// \param[in] _buffer_size The size of \p _buffer.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RcComm* comm = // Our iRODS connection.
///
/// char buf[256];
/// memset(buf, 0, sizeof(buf));
/// fill_with_random_bytes(buf, sizeof(buf));
///
/// assert(set_session_signature_client_side(comm, buf, sizeof(buf)) == 0);
/// assert(strncmp(comm->session_signature, buf, 16) == 0);
/// \endcode
///
/// \since 4.3.1
int set_session_signature_client_side(rcComm_t* _comm, const char* _buffer, size_t _buffer_size);

int
clientLoginWithPassword( rcComm_t *conn, char* password );
void
cliReconnManager( rcComm_t *conn );
int
cliChkReconnAtSendStart( rcComm_t *conn );
int
cliChkReconnAtSendEnd( rcComm_t *conn );
int
cliChkReconnAtReadStart( rcComm_t *conn );
int
cliChkReconnAtReadEnd( rcComm_t *conn );
int
isLoopbackAddress( const char* ip_address );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RC_CONNECT_H
