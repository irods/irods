/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_LIB_FD_HPP
#define	IFUSE_LIB_FD_HPP

#include <pthread.h>
#include "iFuse.Lib.Conn.hpp"
#include "rodsClient.h"

typedef struct IFuseFd {
    unsigned long fdId;
    int fd;
    iFuseConn_t *conn;
    char *iRodsPath;
    int openFlag;
    off_t lastFilePointer;
    pthread_mutexattr_t lockAttr;
    pthread_mutex_t lock;
} iFuseFd_t;

typedef struct IFuseDir {
    unsigned long ddId;
    collHandle_t *handle;
    iFuseConn_t *conn;
    char *iRodsPath;
    pthread_mutexattr_t lockAttr;
    pthread_mutex_t lock;
} iFuseDir_t;

void iFuseFdInit();
void iFuseDirInit();
void iFuseFdDestroy();
void iFuseDirDestroy();
int iFuseFdOpen(iFuseFd_t **iFuseFd, iFuseConn_t *iFuseConn, const char* iRodsPath, int openFlag);
int iFuseFdReopen(iFuseFd_t *iFuseFd);
int iFuseDirOpen(iFuseDir_t **iFuseDir, iFuseConn_t *iFuseConn, const char* iRodsPath);
int iFuseFdClose(iFuseFd_t *iFuseFd);
int iFuseDirClose(iFuseDir_t *iFuseDir);
void iFuseFdLock(iFuseFd_t *iFuseFd);
void iFuseDirLock(iFuseDir_t *iFuseDir);
void iFuseFdUnlock(iFuseFd_t *iFuseFd);
void iFuseDirUnlock(iFuseDir_t *iFuseDir);

#endif	/* IFUSE_LIB_FD_HPP */

