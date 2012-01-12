/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* hpssFileDriver.h - header file for hpssFileDriver.c
 */



#ifndef HPSS_FILE_DRIVER_H
#define HPSS_FILE_DRIVER_H

#include <stdio.h>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>  
#endif
#include <dirent.h>
   
#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#if defined(aix_platform) || defined(sgi_platform)
#include <sys/statfs.h>
#endif
#if defined(osx_platform)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <sys/stat.h>

#include "rods.h"
#include "rcConnect.h"
#include "msParam.h"

#include "u_signed64.h"
#include "hpss_api.h"
#include "mvr_protocol.h"
#include <sys/time.h>
#include "hpss_errno.h"
#include "hpss_String.h"

#define LARGE_SPACE	1000000000
#define HPSS_BUF_SIZE	(2*1024*1024)  /* 2 MB buf size */
#define HPSS_AUTH_FILE	"hpssAuth"

/* definitions for HPSS COS */
#define  HPSS_COS_CONFIG_FILE   "hpssCosConfig"  /* The hpss cos config file */
#define DEF_COS_KW     "default_cos"   /* the keyword for the default cos */

/*  definitions for HpssDefCos */

#define COS_NOT_INIT		-1	/* cos has not been initialized */
#define NO_DEF_COS		-2	/* no default cos */

#define MAX_HPSS_CONNECTIONS	16

typedef struct hpssCosDef {
    int cos;    /* class of service */
    rodsLong_t maxSzInKByte;      /* Max size of the cos in KBytes */
    struct hpssCosDef *next;
} hpssCosDef_t;

typedef struct {
    int	active;      	/* Whether thread/connection is active */
    pthread_t threadId; /* Id of the transfer thread */
    int moverSocket; 	/* mover socket descriptor */
    void *hpssSession;	/* pointer to hpssSession_t */
} hpssThrInfo_t;

/* definition of operation in struct HpssSession */

#define HPSS_GET_OPR	0
#define HPSS_PUT_OPR	1

typedef struct HpssSession {
    u_signed64  fileSize64;
    char 	 *unixFilePath;
    int 	operation;
    int         requestId;      /* HPSS request id */
    int         status; 	/* the session status. HPSS_E_NOERROR if ok */
    int         mySocket;       /* Mover connection socket */
    struct sockaddr_in mySocketAddr;
    unsigned long   ipAddr;	/* ipAddr of local host */
    pthread_mutex_t myMutex;
    pthread_t moverConnManagerThr;
    hpssThrInfo_t thrInfo[MAX_HPSS_CONNECTIONS];
    int         thrCnt;        /* the number of threads */
    u_signed64  totalBytesMoved64; /* Actual bytes sent from Movers */
    int         createFlag; /* First time the file is created */
    int createMode;
} hpssSession_t;

int
hpssFileUnlink (rsComm_t *rsComm, char *filename);
int
hpssFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf);
int
hpssFileMkdir (rsComm_t *rsComm, char *filename, int mode);
int
hpssFileChmod (rsComm_t *rsComm, char *filename, int mode);
int
hpssFileOpendir (rsComm_t *rsComm, char *dirname, void **outDirPtr);
int
hpssFileReaddir (rsComm_t *rsComm, void *dirPtr, struct  dirent *direntPtr);
int
hpssFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName);
rodsLong_t
hpssFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag);
int
hpssFileRmdir (rsComm_t *rsComm, char *filename);
int
hpssFileClosedir (rsComm_t *rsComm, void *dirPtr);
int
hpssStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput);
int
hpssSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType,
int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize,
keyValPair_t *condInput);
int
hpssStatToStat (hpss_stat_t *hpssstat, struct stat *statbuf);
int
initHpssAuth ();
int
readHpssAuthInfo (char *hpssUser, char *hpssAuthInfo);
int
queCos (hpssCosDef_t *myHpssCos);
int
initHpssCos ();
int
getHpssCos (rodsLong_t fileSize);
int
hpssOpenForWrite (char *destHpssFile, int mode, int flags, 
rodsLong_t dataSize);
int
hpssOpenForRead (char *srcHpssFile, int flags);
int
seqHpssPut (char *srcUnixFile, char *destHpssFile, int mode, int flags,
rodsLong_t dataSize);
int
paraHpssPut (char *srcUnixFile, char *destHpssFile, int mode, int flags,
rodsLong_t mySize);
int
seqHpssGet (char *srcHpssFile, char *destUnixFile, int mode, int flags,
rodsLong_t dataSize);
int
initHpssSession (hpssSession_t *hpssSession, int operation, char *unixFilePath,
rodsLong_t fileSize, int createMode);
int
createControlSocket (hpssSession_t *hpssSession);
int
initHpssIodForWrite (hpss_IOD_t *iod, iod_srcsinkdesc_t *srcDesc,
iod_srcsinkdesc_t *sinkDesc, int destFd, hpssSession_t *hpssSession);
void
moverConnManager (hpssSession_t *hpssSession);
void 
getMover (hpssThrInfo_t *thrInfo);
void
putMover (hpssThrInfo_t *thrInfo);
int
paraHpssGet (char *srcHpssFile, char *destUnixFile, int mode, int flags,
rodsLong_t mySize);
int
procMoverInitmsg (hpssThrInfo_t *thrInfo, initiator_msg_t *initMessage,
initiator_msg_t *initReply);
int
procTransferListenSocket (hpssThrInfo_t *thrInfo, initiator_ipaddr_t *ipAddr,
int *transferListenSocket);
int
procTransferSocketFd (hpssThrInfo_t *thrInfo, int transferListenSocket,
int *transferSocketFd);
int
initHpssIodForRead (hpss_IOD_t *iod, iod_srcsinkdesc_t *src,
iod_srcsinkdesc_t *sink, int hpssSrcFd, hpssSession_t *hpssSession);
int
postProcSessionThr (hpssSession_t *myHpssSession, char *hpssPath);

#endif	/* HPSS_FILE_DRIVER_H */
