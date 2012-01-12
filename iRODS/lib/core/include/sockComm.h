/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sockComm.h - header file for sockComm.c
 */

#ifndef SOCK_COMM_H
#define SOCK_COMM_H

#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"

#define MAX_LISTEN_QUE	50
#define SOCK_WINDOW_SIZE	(1*1024*1024)   /* sock window size = 1 Mb */
#define MIN_SOCK_WINDOW_SIZE	(16*1024)   /* min sock window size = 16 kb */
#define MAX_SOCK_WINDOW_SIZE	(16*1024*1024) /* max window size = 16 Mb */
#define DEF_NUMBER_SVR_PORT	200	/* default number of of server ports */
#define CONNECT_TIMEOUT_TIME    100	/* connection timeout time in sec */
#define RECONNECT_WAIT_TIME  100	/* re-connection timeout time in sec */
#define RECONNECT_SLEEP_TIME  300		/* re-connection sleep time in sec */
#define MAX_RECONN_RETRY_CNT 4		/* max connect retry count */
#define MAX_CONN_RETRY_CNT 3	/* max connect retry count */
#define  CONNECT_SLEEP_TIME 200000	/* connect sleep time in uSec */

#define READ_STARTUP_PACK_TOUT_SEC	100	/* 1 sec timeout */
#define READ_VERSION_TOUT_SEC		100	/* 10 sec timeout */
/* definition for the reconnFlag */
#define NO_RECONN	0	/* no reconnection */
#define RECONN_NOTUSED	1	/* this has been depricated */
#define RECONN_TIMEOUT	200

#if 0	/* XXXXXXXXXX testing only */
#define RECONN_TIMEOUT_TIME  600   /* re-connection timeout time in sec */
#else
#define RECONN_TIMEOUT_TIME  600   /* re-connection timeout time in sec */
#endif

#define RECONNECT_ENV "irodsReconnect"		/* reconnFlag will be set to
						 * RECONN_TIMEOUT if this
						 * env is set */

/* definition for socket close function */
#define READING_FROM_CLI	0
#define PROCESSING_API		1

#ifdef _WIN32
#define CLOSE_SOCK       closesocket
#else
#define CLOSE_SOCK       close
#endif

#ifdef  __cplusplus
extern "C" {
#endif

int sockOpenForInConn (rsComm_t *rsComm, int *portNum, char **addr, int proto);
int rodsSetSockOpt (int sock, int windowSize);
int readMsgHeader (int sock, msgHeader_t *myHeader, struct timeval *tv);
int writeMsgHeader (int sock, msgHeader_t *myHeader);
int readVersion (int sock, version_t **myVersion);
int myRead (int sock, void *buf, int len, irodsDescType_t irodsDescType,
int *bytesRead, struct timeval *tv);
int myWrite (int sock, void *buf, int len, irodsDescType_t irodsDescType,
int *bytesWritten);
int connectToRhost (rcComm_t *conn, int connectCnt, int reconnFlag);
int connectToRhostWithRaddr (struct sockaddr_in *remoteAddr, int windowSize,
int timeoutFlag);
int connectToRhostWithTout (int sock, struct sockaddr *sin);
int rodsSleep (int sec, int microSec);
int setConnAddr (rcComm_t *conn);
int setRemoteAddr (int sock, struct sockaddr_in *remoteAddr);
int setLocalAddr (int sock, struct sockaddr_in *localAddr);
int sendStartupPack (rcComm_t *conn, int connectCnt, int reconnFlag);
int sendRodsMsg (int sock, char *msgType, bytesBuf_t *msgBBuf, 
bytesBuf_t *byteStreamBBuf, bytesBuf_t *errorBBuf, int intInfo,
irodsProt_t irodsProt);
int
sendVersion (int sock, int versionStatus, int reconnPort, 
char *reconnAddr, int cookie);
int
readMsgBody (int sock, msgHeader_t *myHeader, bytesBuf_t *inputStructBBuf,
bytesBuf_t *bsBBuf, bytesBuf_t *errorBBuf, irodsProt_t irodsProt,
struct timeval *tv);
int
connectToRhostPortal (char *rodsHost, int rodsPort, int cookie,
int windowSize);
int
rsAcceptConn (rsComm_t *svrComm);
char *
rods_inet_ntoa (struct in_addr in);
int
irodsCloseSock (int sock);
int
readReconMsg (int sock, reconnMsg_t **reconnMsg);
int
sendReconnMsg (int sock, reconnMsg_t *reconnMsg);
int
addUdpPortToPortList (portList_t *thisPortList, int udpport);
int
getUdpPortFromPortList (portList_t *thisPortList);
int
getTcpPortFromPortList (portList_t *thisPortList);
int
addUdpSockToPortList (portList_t *thisPortList, int udpsock);
int
getUdpSockFromPortList (portList_t *thisPortList);
int
getTcpSockFromPortList (portList_t *thisPortList);
int
isReadMsgError (int status);
int svrSwitchConnect (rsComm_t *rsComm);
int cliSwitchConnect (rcComm_t *conn);
int 
redirectConnToRescSvr (rcComm_t **conn, dataObjInp_t *dataObjInp, 
rodsEnv *myEnv, int reconnFlag);
int
rcReconnect (rcComm_t **conn, char *newHost, rodsEnv *myEnv, int reconnFlag);
int
myReadTout (int sock, void *buf, int len, irodsDescType_t irodsDescType,
 int *bytesRead,  struct timeval *tv);
int
mySockClose (int sock);
#ifdef  __cplusplus
}
#endif

#endif	/* SOCK_COMM_H */
