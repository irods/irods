/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* miscServerFunct.h - header file for miscServerFunct.c
 */



#ifndef MISC_SERVER_FUNCT_H
#define MISC_SERVER_FUNCT_H

#include <sys/types.h>

#ifndef _WIN32
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "rods.h"
#include "rcConnect.h"
#include "initServer.h"
#include "fileOpen.h"
#include "dataObjInpOut.h"
#include "dataCopy.h"
#ifdef RBUDP_TRANSFER
#include "QUANTAnet_rbudpBase_c.h"
#include "QUANTAnet_rbudpSender_c.h"
#include "QUANTAnet_rbudpReceiver_c.h"
#endif	/* RBUDP_TRANSFER */

#include "structFileSync.h" /* JMC */

#define MAX_RECON_ERROR_CNT	10

typedef struct PortalTransferInp {
    rsComm_t *rsComm;
    int destFd;
    int srcFd;
    int destRescTypeInx;
    int srcRescTypeInx;
    int threadNum;
    rodsLong_t size;
    rodsLong_t offset;
    rodsLong_t bytesWritten;
    int flags;
    int status;
    dataOprInp_t *dataOprInp;
} portalTransferInp_t;

int
svrToSvrConnect (rsComm_t *rsComm, rodsServerHost_t *rodsServerHost);
int
svrToSvrConnect (rsComm_t *rsComm, rodsServerHost_t *rodsServerHost);
int
svrToSvrConnectNoLogin (rsComm_t *rsComm, rodsServerHost_t *rodsServerHost);
int
createSrvPortal (rsComm_t *rsComm, portList_t *thisPortList, int proto);
int
acceptSrvPortal (rsComm_t *rsComm, portList_t *thisPortList);
int
svrPortalPutGet (rsComm_t *rsComm);
void
partialDataPut (portalTransferInp_t *myInput);
void
partialDataGet (portalTransferInp_t *myInput);
int
fillPortalTransferInp (portalTransferInp_t *myInput, rsComm_t *rsComm,
int srcFd, int destFd, int destRescTypeInx, int srcRescTypeInx,
int threadNum, rodsLong_t size, rodsLong_t offset, int flags);
int
sameHostCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
void
sameHostPartialCopy (portalTransferInp_t *myInput);
int
rbudpRemLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
int
remLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
void
remToLocPartialCopy (portalTransferInp_t *myInput);
void
locToRemPartialCopy (portalTransferInp_t *myInput);
int
singleRemLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
int
singleRemToLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
int
singleLocToRemCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp);
int
isUserPrivileged(rsComm_t *rsComm);
#if !defined(solaris_platform)
char *regcmp (char *pat, char *end);
char *regex (char *rec, char *text, ...);
#endif
#ifdef  __cplusplus
int intNoSupport( ... );
rodsLong_t longNoSupport( ... );
#else
int intNoSupport();
rodsLong_t longNoSupport();
#endif
void getZoneServerId(char *zoneName, char *zoneSID);
#ifdef RBUDP_TRANSFER
int
svrPortalPutGetRbudp (rsComm_t *rsComm);
#endif	/* RBUDP_TRANSFER */
#ifndef windows_platform
void
reconnManager (rsComm_t *rsComm);
int
svrChkReconnAtReadStart (rsComm_t *rsComm);
int
svrChkReconnAtReadEnd (rsComm_t *rsComm);
int
svrChkReconnAtSendStart (rsComm_t *rsComm);
int 
svrChkReconnAtSendEnd (rsComm_t *rsComm);
#endif
int
svrSockOpenForInConn (rsComm_t *rsComm, int *portNum, char **addr, int proto);
char *
getLocalSvrAddr ();
char *
_getSvrAddr (rodsServerHost_t *rodsServerHost);
char *
getSvrAddr (rodsServerHost_t *rodsServerHost);
int
setLocalSrvAddr (char *outLocalAddr);
int
forkAndExec (char *av[]);
int
setupSrvPortalForParaOpr (rsComm_t *rsComm, dataOprInp_t *dataOprInp,
int oprType, portalOprOut_t **portalOprOut);
int
readStartupPack (int sock, startupPack_t **startupPack, struct timeval *tv);
#ifdef RUN_SERVER_AS_ROOT
int 
initServiceUser ();
int
isServiceUserSet ();
int
changeToRootUser ();
int 
changeToServiceUser ();
#endif
#endif	/* MISC_SERVER_FUNCT_H */
