/**
 * @file  rodsDef.h
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsdef.h - common header file for rods
 */



#ifndef RODS_DEF_HPP
#define RODS_DEF_HPP

#include <stdio.h>
#include <errno.h>
#if defined(solaris_platform)
#include <fcntl.h>
#endif
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>



#if defined(solaris_platform)
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include "dirent.hpp"
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif


#define HEADER_TYPE_LEN 128 /* changed by Raja to 128 from 16 */
#define TIME_LEN        32
#define NAME_LEN        64
#define CHKSUM_LEN      64  /* keep chksum_len the same as the old size */
#define LONG_NAME_LEN	256
#define MAX_PATH_ALLOWED 1024
/* MAX_NAME_LEN actually has space for a few extra characters to match
   the way it is often used in the code. */
#define MAX_NAME_LEN   (MAX_PATH_ALLOWED+64)
#define HUGE_NAME_LEN 100000

/* increase to 20K because the French Lib rules are very long */
#define META_STR_LEN  (1024*20)

#define SHORT_STR_LEN 32  /* for dataMode, perhaps others */

#ifndef NULL
#define NULL    0
#endif

#define PTR_ARRAY_MALLOC_LEN    10	/* the len to allocate each time */

/* definition for the global variable int ProcessType */

#define CLIENT_PT	0	/* client process type */
#define SERVER_PT	1	/* server process type */
#define AGENT_PT	2	/* agent process type */
#define RE_SERVER_PT	3	/* reServer type */
#define XMSG_SERVER_PT	4	/* xmsgServer type */

/* definition for rcat type */

#define MASTER_RCAT	0
#define SLAVE_RCAT	1
#define MAX_SZ_FOR_SINGLE_BUF     (32*1024*1024)
#define INIT_SZ_FOR_EXECMD_BUF     (16*1024)
#define MAX_SZ_FOR_EXECMD_BUF     (1*1024*1024)
#define MIN_SZ_FOR_PARA_TRAN     (1*1024*1024)
#define TRANS_BUF_SZ    (4*1024*1024)
#define CRYPT_BUF_SZ    2*TRANS_BUF_SZ
#define TRANS_SZ        (40*1024*1024)
#define LARGE_SPACE     1000000000
#define UNKNOWN_FILE_SZ	-99	/* value to indicate the file sz is unknown */
#define MIN_RESTART_SIZE	(64*1024*1024)
#define RESTART_FILE_UPDATE_SIZE  (32*1024*1024)

#define DEF_NUM_TRAN_THR        4
#define MAX_NUM_CONFIG_TRAN_THR        64
#define SZ_PER_TRAN_THR         (32*1024*1024)

/* definition for numThreads input */

#define NO_THREADING	-1
#define AUTO_THREADING   0	/* default */

#define NO_SAVE_REI  0
#define SAVE_REI  1

#define SELECT_TIMEOUT_FOR_CONN	60	/* 60 sec wait for connection */
/* this is the return value for the rcExecMyRule call indicating the
 * server is requesting the client to client to perform certain task */
#define SYS_SVR_TO_CLI_MSI_REQUEST 99999995
#define SYS_SVR_TO_CLI_COLL_STAT 99999996
#define SYS_CLI_TO_SVR_COLL_STAT_REPLY 99999997
#define SYS_SVR_TO_CLI_PUT_ACTION 99999990
#define SYS_SVR_TO_CLI_GET_ACTION 99999991
#define SYS_RSYNC_TARGET_MODIFIED 99999992	/* target modified */

/* definition for iRODS server to client action request from a microservice.
 * these definitions are put in the "label" field of MsParam */

#define CL_PUT_ACTION	"CL_PUT_ACTION"
#define CL_GET_ACTION	"CL_GET_ACTION"
#define CL_ZONE_OPR_INX	"CL_ZONE_OPR_INX"

/* the following defines the RSYNC_MODE_KW */
#define LOCAL_TO_IRODS	"LOCAL_TO_IRODS"
#define IRODS_TO_LOCAL	"IRODS_TO_LOCAL"
#define IRODS_TO_IRODS	"IRODS_TO_IRODS"
#define IRODS_TO_COLLECTION	"IRODS_TO_COLLECTION"

/* definition for public user */
#define PUBLIC_USER_NAME	"public"

/* definition for anonymous user */
#define ANONYMOUS_USER "anonymous"

/* definition for bulk operation */
#define MAX_NUM_BULK_OPR_FILES	50
#define MAX_BULK_OPR_FILE_SIZE  (4*1024*1024)
#define BULK_OPR_BUF_SIZE	(8*MAX_BULK_OPR_FILE_SIZE)
#define TAR_OVERHEAD		(MAX_NUM_BULK_OPR_FILES * MAX_NAME_LEN * 2)

/* definition for SYS_TIMING */
/* #define SYS_TIMING	1 */	/* switch on or offf SYS_TIMING */
#define SYS_TIMING_SEC     "sysTimingSec"    /* timing env var in sec */
#define SYS_TIMING_USEC    "sysTimingUSec"   /* timing env var in micro sec */

/* generic return value for policy rules */
#define POLICY_OFF	0
#define POLICY_ON	1

/* protocol */
typedef enum {
    NATIVE_PROT,
    XML_PROT
} irodsProt_t;

/* myRead/myWrite type */

typedef enum {
    SOCK_TYPE,
    FILE_DESC_TYPE
} irodsDescType_t;

/* position for queuing */
typedef enum {
    BOTTOM_POS,
    TOP_POS
} irodsPosition_t;

typedef enum {
    UNINIT_STATE,
    OFF_STATE,
    ON_STATE
} irodsStateFlag_t;

typedef enum {
    NOT_ORPHAN_PATH,
    IS_ORPHAN_PATH,
    is_ORPHAN_HOME
} orphanPathType_t;

#define DEF_IRODS_PROT	NATIVE_PROT

/**
 * \var bytesBuf_t
 * \brief  general struct to store a buffer of bytes
 * \since 1.0
 *
 * \remark none
 *
 * \note
 * Elements of bytesBuf_t:
 * \li int len - the length of the allocated buffer in buf or the length of
 *        the data stored in buf depending on the application.
 * \li void *buf - pointer to the buffer.
 *
 * \sa none
 * \bug  no known bugs
 */

typedef struct BytesBuf {   /* have to add BytesBuf to get Doxygen working */
    int len;	/* len in bytes in buf */
    void *buf;
} bytesBuf_t;

typedef struct {   /* have to add BytesBuf to get Doxygen working */
    int type;
    int len;    /* len of array in buf */
    void *buf;
} dataArray_t;

/* The msg header for all communication between client and server */

typedef struct msgHeader {
    char type[HEADER_TYPE_LEN];
    int msgLen;     /* Length of the main msg */
    int errorLen;   /* Length of the error struct */
    int bsLen;      /* Length of optional byte stream */
    int intInfo;    /* an additional integer info, for API, it is the
			 * apiReqNum */
} msgHeader_t;

/* header length XML tag */
#define MSG_HEADER_LEN_TAG	"MsgHeaderLen"

/* msg type */
#define RODS_CONNECT_T    "RODS_CONNECT"
#define RODS_VERSION_T    "RODS_VERSION"
#define RODS_API_REQ_T    "RODS_API_REQ"
#define RODS_DISCONNECT_T    "RODS_DISCONNECT"
#define RODS_RECONNECT_T    "RODS_RECONNECT"
#define RODS_REAUTH_T     "RODS_REAUTH"
#define RODS_API_REPLY_T    "RODS_API_REPLY"
#define RODS_CS_NEG_T    "RODS_CS_NEG_T"

/* The strct sent with RODS_CONNECT type by client */
typedef struct startupPack {
    irodsProt_t irodsProt;
    int reconnFlag;
    int connectCnt;
    char proxyUser[NAME_LEN];
    char proxyRodsZone[NAME_LEN];
    char clientUser[NAME_LEN];
    char clientRodsZone[NAME_LEN];
    char relVersion[NAME_LEN];
    char apiVersion[NAME_LEN];
    char option[NAME_LEN];
} startupPack_t;

/* env variable for the client protocol */
#define IRODS_PROT	"irodsProt"

/* env variable for the startup pack */

#define SP_NEW_SOCK	"spNewSock"
#define SP_CLIENT_ADDR	"spClientAddr"
#define SP_CONNECT_CNT	"spConnectCnt"
#define SP_PROTOCOL	"spProtocol"
#define SP_RECONN_FLAG	"spReconnFlag"
#define SP_PROXY_USER	"spProxyUser"
#define SP_PROXY_RODS_ZONE "spProxyRodsZone"
#define SP_CLIENT_USER	"spClientUser"
#define SP_CLIENT_RODS_ZONE "spClientRodsZone"
#define SP_REL_VERSION	"spRelVersion"
#define SP_API_VERSION	"spApiVersion"
#define SP_OPTION	"spOption"
#define SP_LOG_SQL	"spLogSql"
#define SP_LOG_LEVEL	"spLogLevel"
#define SP_RE_CACHE_SALT "reCacheSalt"
#define SERVER_BOOT_TIME "serverBootTime"

// =-=-=-=-=-=-=-
// magic token to assign to startup pack option variable
// in order to request a client-server negotiation
#define REQ_SVR_NEG	"request_server_negotiation"

/* Definition for resource status. If it is empty (strlen == 0), it is
 * assumed to be up */
#define RESC_DOWN	"down"
#define RESC_UP		"up"
#define RESC_AUTO_UP	"auto-up"
#define RESC_AUTO_DOWN	"auto-down"

#define  INT_RESC_STATUS_UP 	0
#define  INT_RESC_STATUS_DOWN 	1

/* The strct sent with RODS_VERSION type by server */

typedef struct {
    int status;		/* if > 0, contains the reconnection port */
    char relVersion[NAME_LEN];
    char apiVersion[NAME_LEN];
    int reconnPort;
    char reconnAddr[LONG_NAME_LEN];
    int cookie;
} version_t;
/* struct that defines a Host Addr */

typedef struct {
    char hostAddr[LONG_NAME_LEN];
    char zoneName[NAME_LEN];
    int portNum;
    int dummyInt;	/* make it to 64 bit boundary */
} rodsHostAddr_t;

/* definition for restartState */

#define PATH_MATCHING		0x1
#define LAST_PATH_MATCHED	0x2
#define MATCHED_RESTART_COLL    0x4
#define OPR_RESUMED    		0x8

/* definition for DNN WOS connection env */

#define WOS_HOST_ENV    "wosHost"
#define WOS_POLICY_ENV  "wosPolicy"

/* struct that defines restart operation */

typedef struct {
    char restartFile[MAX_NAME_LEN];
    int fd;		/* df of the opened restartFile */
    int doneCnt;	/* a count of number of files done in the coll/dir */
    char collection[MAX_NAME_LEN];      /* the coll/dir being restarted */
    char lastDonePath[MAX_NAME_LEN];	/* the last path that is done */
    char oprType[NAME_LEN];	/* BULK_OPR_KW or NON_BULK_OPR_KW */
    int curCnt;
    int restartState;
} rodsRestart_t;

/* definition for handler function */
#ifdef __cplusplus
typedef int( ( *funcPtr )( ... ) );
#else
typedef int( ( *funcPtr )( ) );
#endif

/* some platform does not support vfork */
#if defined(sgi_platform) || defined(aix_platform)
#define RODS_FORK() fork()
#else
#define RODS_FORK() vfork()
#endif

#define VAULT_PATH_POLICY	"VAULT_PATH_POLICY"	/* msParam lable */
/* definition for vault filePath scheme */
typedef enum {
    GRAFT_PATH_S,
    RANDOM_S
} vaultPathScheme_t;

#define DEF_VAULT_PATH_SCHEME	GRAFT_PATH_S
#define DEF_ADD_USER_FLAG	1
#define DEF_TRIM_DIR_CNT	1

typedef struct {
    vaultPathScheme_t scheme;
    int addUserName;
    int trimDirCnt;	/* for GRAFT_PATH_S only. Number of directories to
			 * trim */
} vaultPathPolicy_t;

/* struct for proc (agent) logging */
typedef struct {
    int pid;
    unsigned int startTime;
    char clientName[NAME_LEN];
    char clientZone[NAME_LEN];
    char proxyName[NAME_LEN];
    char proxyZone[NAME_LEN];
    char remoteAddr[NAME_LEN];
    char serverAddr[NAME_LEN];
    char progName[NAME_LEN];
} procLog_t;

#endif	/* RODS_DEF_H */
