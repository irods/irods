/**
 * @file srbmso.h
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>


#define MESSAGE_SIZE 1000
#define COMM_BUF_SIZE 512
#define ERROR_MSG_LENGTH 4096
#define CLI_CONNECTION_OK 0
#define BUFSIZE        (2 * 1024 *1024)    /* 2 Mb */
#define SRB_O_TRUNC 0x200



typedef  rodsLong_t  srb_long_t;





typedef enum _MsgType {
  ACK_MSG = 0,                /* acknowledge a message */
  ERROR_MSG=1,                /* error response to client from server */
  RESET_MSG=2,                /* client must reset connection */
  PRINT_MSG=3,                /* tuples for client from server */
  NET_ERROR=4,                /* error in net system call */
  FUNCTION_MSG=5,             /* fastpath call (unused) */
  QUERY_MSG=6,                /* client query to server */
  STARTUP_MSG=7,              /* initialize a connection with a backend */
  DUPLICATE_MSG=8,            /* duplicate msg arrived (errors msg only) */
  INVALID_MSG=9,              /* for some control functions */
  STARTUP_D_AUTH_MSG=10       /* use the default authentication (host based) */
} MsgType;

typedef struct PacketBuf {
#if defined(PORTNAME_c90)
  int len:32;
  MsgType msgtype:32;
#else
  int len;
  MsgType msgtype;
#endif
  char data[MESSAGE_SIZE];
} PacketBuf;

struct portStrut {
  int                 sock;   /* file descriptor */
  int                 pid;    /* the pid of srbServer */
  int                 srbPort; /* The port number of the srbServer */
  int                 pipeFd1;   /* the pipe fd to reply to srbServer */
  int                 nBytes; /* nBytes read in so far */
  int                 preSpawn; /* preSpawn srbServer ? */
  struct sockaddr_in  laddr;  /* local addr (us) */
  struct sockaddr_in  raddr;  /* remote addr (them) */
  PacketBuf           buf;    /* stream implementation (curr pack buf) */
  struct portStrut *next;
};

typedef struct portStrut Port;



typedef struct {
  int inpInx1;
  int inpInx2;
  int outInx;
  unsigned char inbuf[COMM_BUF_SIZE];
  char outbuf[COMM_BUF_SIZE];
} comBuf;


typedef struct svrComm {
  int commFd;
  int inpInx1;
  int inpInx2;
  int outInx;
  int delegateCertInx;
  unsigned char inBuf[COMM_BUF_SIZE];    /* use of unsigned is important.
					  * If not, can trigger EOF in
					  * commGetc when reading -ive int
					  */
  char outBuf[COMM_BUF_SIZE];
} svrComm_t;

/* Client user info */

typedef struct userInfo {
  char *userName;       /* the user ID */
  char *domainName;     /* The user domain */
  char *mcatZone;       /* The user mcat zone */
  char *userAuth;       /* Authentication string (mdas/sea passwd) */
  int   userId;         /* the numeric user id */
  int   privUserFlag;   /* Indicate whether the user is privileged,
                         * 0-no, 1-yes */
  int authSchemeInx;
  svrComm_t *myComm;    /* set this to NULL if multithread. Used by elog()
                         * to set client msg
                         */
  int session;
  void *sessionBuf;
} userInfo;


typedef struct exf_conn{
  char *srbHost; /* the machine on which the server is running */
  char *srbVersion;  /* version of srb */
  char *srbPort; /* the communication port with the backend */
  char *srbOptions; /* options to start the backend with */
  userInfo *proxyUser;
  userInfo *clientUser;
  char *srbMdasDomain; /* authorization string */
  int status;
  char errorMessage[ERROR_MSG_LENGTH*8];
  /* pipes for be/fe communication */
  FILE *Pfin;
  FILE *Pfout;
  FILE *Pfdebug;
  comBuf *commBuf;
  int sock;
  int sockIn;
  Port *port; /* really a Port* */
  int numThread;
  int authSchemeInx;    /* the auth scheme to use - PASSWD_AUTH_INX,
                         * SEA_AUTH_INX, SEA_ENCRYPT_INX,
                         * GSI_AUTH_INX or GSI_SECURE_COMM_INX.
                         */
  char *dn;             /* the server dn on the other side */
} srbConn;

extern srbConn* srbConnect(char *srbHost, char* srbPort, char* srbAuth, char *userName,
	   char *domainName, char *authScheme,  char *serverDn);
extern void clFinish(srbConn* conn);
extern int srbObjOpen(srbConn* conn, char *objID, int oflag,
		      char *collectionName);
extern int srbObjRead(srbConn *conn, int desc, char *buf, int len);
extern int srbObjClose (srbConn* conn, int desc);
extern int srbObjWrite(srbConn *conn, int desc, char *buf, int len);
extern int clStatus(srbConn* conn);
