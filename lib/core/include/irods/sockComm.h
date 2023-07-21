#ifndef IRODS_SOCK_COMM_H
#define IRODS_SOCK_COMM_H

/// \file

#include "irods/rodsDef.h"
#include "irods/rodsPackInstruct.h"

struct RcComm;
struct RsComm;
struct PortList;
struct DataObjInp;
struct RodsEnvironment;

#define MAX_LISTEN_QUE                  50
#define DEF_NUMBER_SVR_PORT             200                 /* default number of of server ports */
#define CONNECT_TIMEOUT_TIME            100                 /* connection timeout time in sec */
#define RECONNECT_WAIT_TIME             100                 /* re-connection timeout time in sec */
#define RECONNECT_SLEEP_TIME            300                 /* re-connection sleep time in sec */
#define MAX_RECONN_RETRY_CNT            4                   /* max connect retry count */
#define MAX_CONN_RETRY_CNT              3                   /* max connect retry count */
#define CONNECT_SLEEP_TIME              200000              /* connect sleep time in uSec */

#define READ_STARTUP_PACK_TOUT_SEC      100                 /* 1 sec timeout */
#define READ_VERSION_TOUT_SEC           100                 /* 10 sec timeout */

#define RECONNECT_ENV                   "irodsReconnect"    /* reconnFlag will be set to
                                                             * RECONN_TIMEOUT if this
                                                             * env is set */

/* definition for socket close function */
#define READING_FROM_CLI                0
#define PROCESSING_API                  1

#ifdef _WIN32
    #define CLOSE_SOCK                  closesocket
#else
    #define CLOSE_SOCK                  close
#endif

#ifdef __cplusplus
extern "C" {
#endif

// =-=-=-=-=-=-=-
// other legacy functions
int sockOpenForInConn(struct RsComm *comm, int *portNum, char **addr, int proto);

int rodsSetSockOpt(int sock, int tcp_buffer_size);

int connectToRhost(struct RcComm *comm, int connectCnt, int reconnFlag);

int connectToRhostWithRaddr(struct sockaddr_in *remoteAddr, int windowSize, int timeoutFlag);

int connectToRhostWithTout(struct sockaddr *sin);

int rodsSleep(int sec, int microSec);

int setConnAddr(struct RcComm *comm);

int setRemoteAddr(int sock, struct sockaddr_in *remoteAddr);

int setLocalAddr(int sock, struct sockaddr_in *localAddr);

int sendStartupPack(struct RcComm *comm, int connectCnt, int reconnFlag);

int connectToRhostPortal(char *rodsHost, int rodsPort, int cookie, int windowSize);

int rsAcceptConn(struct RsComm *comm);

int irodsCloseSock(int sock);

int addUdpPortToPortList(struct PortList *thisPortList, int udpport);

int getUdpPortFromPortList(struct PortList *thisPortList);

int getTcpPortFromPortList(struct PortList *thisPortList);

int addUdpSockToPortList(struct PortList *thisPortList, int udpsock);

int getUdpSockFromPortList(struct PortList *thisPortList);

int getTcpSockFromPortList(struct PortList *thisPortList);

int isReadMsgError(int status);

int svrSwitchConnect(struct RsComm *comm);

int cliSwitchConnect(struct RcComm *comm);

int redirectConnToRescSvr(struct RcComm **comm, struct DataObjInp *dataObjInp, struct RodsEnvironment *myEnv, int reconnFlag);

int rcReconnect(struct RcComm **comm, char *newHost, struct RodsEnvironment *myEnv, int reconnFlag);

int mySockClose(int sock); // server stop fcn <==> rsAccept?

/// \brief Set the TCP_KEEPALIVE options for the specified socket.
///
/// param[in] _sfd Socket file descriptor on which options are being set.
///
/// \return An integer
/// \retval 0 on success
/// \retval <0 on failure
///
/// \since 4.3.1
int set_socket_tcp_keepalive_options(int _sfd); // NOLINT(modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif // IRODS_SOCK_COMM_H
