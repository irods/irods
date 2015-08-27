/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <list>
#include "iFuse.Lib.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "iFuse.Lib.Fd.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "iFuse.Lib.Util.hpp"
#include "sockComm.h"
#include "miscUtil.h"

static pthread_mutexattr_t g_AssignedFdLockAttr;
static pthread_mutex_t g_AssignedFdLock;
static std::list<iFuseFd_t*> g_AssignedFd;

static pthread_mutexattr_t g_AssignedDirLockAttr;
static pthread_mutex_t g_AssignedDirLock;
static std::list<iFuseDir_t*> g_AssignedDir;

static unsigned long g_fdIDGen;
static unsigned long g_ddIDGen;

/*
 * Lock order : 
 * - g_AssignedFdLock or g_AssignedDirLock
 * - iFuseFd_t or iFuseDir_t
 */

static unsigned long _genNextFdID() {
    return g_fdIDGen++;
}

static unsigned long _genNextDdID() {
    return g_ddIDGen++;
}

static int _closeFd(iFuseFd_t *iFuseFd) {
    int status = 0;
    openedDataObjInp_t dataObjCloseInp;
    
    assert(iFuseFd != NULL);
    
    pthread_mutex_lock(&iFuseFd->lock);

    if(iFuseFd->fd > 0 && iFuseFd->conn != NULL) {
        iFuseConnLock(iFuseFd->conn);
        
        bzero(&dataObjCloseInp, sizeof (openedDataObjInp_t));
        dataObjCloseInp.l1descInx = iFuseFd->fd;

        status = iFuseRodsClientDataObjClose(iFuseFd->conn->conn, &dataObjCloseInp);
        if (status < 0) {
            if (iFuseRodsClientReadMsgError(status)) {
                // reconnect and retry 
                if(iFuseConnReconnect(iFuseFd->conn) < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdClose: iFuseConnReconnect of %s (%d) error",
                        iFuseFd->iRodsPath, iFuseFd->fd);
                } else {
                    status = iFuseRodsClientDataObjClose(iFuseFd->conn->conn, &dataObjCloseInp);
                    if (status < 0) {
                        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdClose: close of %s (%d) error",
                            iFuseFd->iRodsPath, iFuseFd->fd);
                    }
                }
            } else {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdClose: close of %s (%d) error",
                    iFuseFd->iRodsPath, iFuseFd->fd);
            }
        }

        iFuseConnUnlock(iFuseFd->conn);
    }
    
    iFuseFd->conn = NULL;
    iFuseFd->fd = 0;
    iFuseFd->openFlag = 0;
    iFuseFd->lastFilePointer = -1;
    
    pthread_mutex_unlock(&iFuseFd->lock);
    return status;
}

static int _closeDir(iFuseDir_t *iFuseDir) {
    int status = 0;
    
    assert(iFuseDir != NULL);
    
    pthread_mutex_lock(&iFuseDir->lock);

    if(iFuseDir->handle != NULL && iFuseDir->conn != NULL) {
        iFuseConnLock(iFuseDir->conn);
        status = iFuseRodsClientCloseCollection(iFuseDir->handle);
        if (status < 0) {
            if (iFuseRodsClientReadMsgError(status)) {
                // reconnect and retry 
                if(iFuseConnReconnect(iFuseDir->conn) < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseDirClose: iFuseConnReconnect of %s error",
                        iFuseDir->iRodsPath);
                } else {
                    status = iFuseRodsClientCloseCollection(iFuseDir->handle);
                    if (status < 0) {
                        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseDirClose: iFuseRodsClientCloseCollection of %s error",
                            iFuseDir->iRodsPath);
                    }
                }
            } else {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseDirClose: iFuseRodsClientCloseCollection of %s error",
                    iFuseDir->iRodsPath);
            }
        }

        iFuseConnUnlock(iFuseDir->conn);
    }
    
    iFuseDir->conn = NULL;
    
    if(iFuseDir->handle != NULL) {
        free(iFuseDir->handle);
        iFuseDir->handle = NULL;
    }
    
    pthread_mutex_unlock(&iFuseDir->lock);
    return status;
}

static int _freeFd(iFuseFd_t *iFuseFd) {
    assert(iFuseFd != NULL);
    
    _closeFd(iFuseFd);
    
    pthread_mutex_destroy(&iFuseFd->lock);
    pthread_mutexattr_destroy(&iFuseFd->lockAttr);
    
    if(iFuseFd->iRodsPath != NULL) {
        free(iFuseFd->iRodsPath);
        iFuseFd->iRodsPath = NULL;
    }

    free(iFuseFd);
    return 0;
}

static int _freeDir(iFuseDir_t *iFuseDir) {
    assert(iFuseDir != NULL);
    
    _closeDir(iFuseDir);
    
    pthread_mutex_destroy(&iFuseDir->lock);
    pthread_mutexattr_destroy(&iFuseDir->lockAttr);
    
    if(iFuseDir->iRodsPath != NULL) {
        free(iFuseDir->iRodsPath);
        iFuseDir->iRodsPath = NULL;
    }
    
    if(iFuseDir->handle != NULL) {
        free(iFuseDir->handle);
        iFuseDir->handle = NULL;
    }

    free(iFuseDir);
    return 0;
}

static int _closeAllFd() {
    iFuseFd_t *iFuseFd;
    
    pthread_mutex_lock(&g_AssignedFdLock);

    // close all opened file descriptors
    while(!g_AssignedFd.empty()) {
        iFuseFd = g_AssignedFd.front();
        g_AssignedFd.pop_front();
        
        _freeFd(iFuseFd);
    }
    
    pthread_mutex_unlock(&g_AssignedFdLock);
    return 0;
}

static int _closeAllDir() {
    iFuseDir_t *iFuseDir;

    pthread_mutex_lock(&g_AssignedDirLock);
    
    // close all opened dir descriptors
    while(!g_AssignedDir.empty()) {
        iFuseDir = g_AssignedDir.front();
        g_AssignedDir.pop_front();
        
        _freeDir(iFuseDir);
    }
    
    pthread_mutex_unlock(&g_AssignedDirLock);
    return 0;
}

/*
 * Initialize file descriptor manager
 */
void iFuseFdInit() {
    pthread_mutexattr_init(&g_AssignedFdLockAttr);
    pthread_mutexattr_settype(&g_AssignedFdLockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_AssignedFdLock, &g_AssignedFdLockAttr);
    
    g_fdIDGen = 0;
}

/*
 * Initialize directory descriptor manager
 */
void iFuseDirInit() {
    pthread_mutexattr_init(&g_AssignedDirLockAttr);
    pthread_mutexattr_settype(&g_AssignedDirLockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_AssignedDirLock, &g_AssignedDirLockAttr);
    
    g_ddIDGen = 0;
}

/*
 * Destroy file descriptor manager
 */
void iFuseFdDestroy() {
    g_fdIDGen = 0;
    
    _closeAllFd();
    
    pthread_mutex_destroy(&g_AssignedFdLock);
    pthread_mutexattr_destroy(&g_AssignedFdLockAttr);
}

/*
 * Destroy directory descriptor manager
 */
void iFuseDirDestroy() {
    g_ddIDGen = 0;
    
    _closeAllDir();
    
    pthread_mutex_destroy(&g_AssignedDirLock);
    pthread_mutexattr_destroy(&g_AssignedDirLockAttr);
}

/*
 * Open a new file descriptor
 */
int iFuseFdOpen(iFuseFd_t **iFuseFd, iFuseConn_t *iFuseConn, const char* iRodsPath, int openFlag) {
    int status = 0;
    dataObjInp_t dataObjOpenInp;
    int fd;
    iFuseFd_t *tmpIFuseDesc;

    assert(iFuseFd != NULL);
    assert(iFuseConn != NULL);
    assert(iRodsPath != NULL);
    
    *iFuseFd = NULL;

    pthread_mutex_lock(&g_AssignedFdLock);
    iFuseConnLock(iFuseConn);
    
    bzero(&dataObjOpenInp, sizeof ( dataObjInp_t));
    dataObjOpenInp.openFlags = openFlag;
    rstrcpy(dataObjOpenInp.objPath, iRodsPath, MAX_NAME_LEN);

    assert(iFuseConn->conn != NULL);
    
    fd = iFuseRodsClientDataObjOpen(iFuseConn->conn, &dataObjOpenInp);
    if (fd <= 0) {
        if (iFuseRodsClientReadMsgError(fd)) {
            // reconnect and retry 
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdOpen: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, fd);
                iFuseConnUnlock(iFuseConn);
                pthread_mutex_unlock(&g_AssignedFdLock);
                return -ENOENT;
            } else {
                fd = iFuseRodsClientDataObjOpen(iFuseConn->conn, &dataObjOpenInp);
                if (fd <= 0) {
                    iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdOpen: iFuseRodsClientDataObjOpen of %s error, status = %d",
                        iRodsPath, fd);
                    iFuseConnUnlock(iFuseConn);
                    pthread_mutex_unlock(&g_AssignedFdLock);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdOpen: iFuseRodsClientDataObjOpen of %s error, status = %d",
                iRodsPath, fd);
            iFuseConnUnlock(iFuseConn);
            pthread_mutex_unlock(&g_AssignedFdLock);
            return -ENOENT;
        }
    }
    
    iFuseConnUnlock(iFuseConn);
    
    tmpIFuseDesc = (iFuseFd_t *) calloc(1, sizeof ( iFuseFd_t));
    if (tmpIFuseDesc == NULL) {
        *iFuseFd = NULL;
        pthread_mutex_unlock(&g_AssignedFdLock);
        return SYS_MALLOC_ERR;
    }
    
    tmpIFuseDesc->fdId = _genNextFdID();
    tmpIFuseDesc->conn = iFuseConn;
    tmpIFuseDesc->fd = fd;
    tmpIFuseDesc->iRodsPath = strdup(iRodsPath);
    tmpIFuseDesc->openFlag = openFlag;
    tmpIFuseDesc->lastFilePointer = -1;
    
    pthread_mutexattr_init(&tmpIFuseDesc->lockAttr);
    pthread_mutexattr_settype(&tmpIFuseDesc->lockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tmpIFuseDesc->lock, &tmpIFuseDesc->lockAttr);
    
    *iFuseFd = tmpIFuseDesc;
    
    g_AssignedFd.push_back(tmpIFuseDesc);
    
    pthread_mutex_unlock(&g_AssignedFdLock);
    return status;
}

/*
 * Close and Reopen a file descriptor
 */
int iFuseFdReopen(iFuseFd_t *iFuseFd) {
    int status = 0;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInp_t dataObjOpenInp;
    int fd;
    
    assert(iFuseFd != NULL);
    assert(iFuseFd->conn != NULL);
    assert(iFuseFd->fd > 0);

    pthread_mutex_lock(&iFuseFd->lock);
    
    iFuseConnLock(iFuseFd->conn);

    bzero(&dataObjCloseInp, sizeof (openedDataObjInp_t));
    dataObjCloseInp.l1descInx = iFuseFd->fd;

    status = iFuseRodsClientDataObjClose(iFuseFd->conn->conn, &dataObjCloseInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            // reconnect and retry 
            if(iFuseConnReconnect(iFuseFd->conn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdReopen: iFuseConnReconnect of %s (%d) error",
                    iFuseFd->iRodsPath, iFuseFd->fd);
            } else {
                status = iFuseRodsClientDataObjClose(iFuseFd->conn->conn, &dataObjCloseInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdReopen: close of %s (%d) error",
                        iFuseFd->iRodsPath, iFuseFd->fd);
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdReopen: close of %s (%d) error",
                iFuseFd->iRodsPath, iFuseFd->fd);
        }
    }
    
    if(status < 0) {
        iFuseConnUnlock(iFuseFd->conn);
        pthread_mutex_unlock(&iFuseFd->lock);
        return status;
    }
    
    iFuseFd->lastFilePointer = -1;

    bzero(&dataObjOpenInp, sizeof ( dataObjInp_t));
    dataObjOpenInp.openFlags = iFuseFd->openFlag;
    rstrcpy(dataObjOpenInp.objPath, iFuseFd->iRodsPath, MAX_NAME_LEN);

    assert(iFuseFd->conn->conn != NULL);
    
    fd = iFuseRodsClientDataObjOpen(iFuseFd->conn->conn, &dataObjOpenInp);
    if (fd <= 0) {
        if (iFuseRodsClientReadMsgError(fd)) {
            // reconnect and retry 
            if(iFuseConnReconnect(iFuseFd->conn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdReopen: iFuseConnReconnect of %s error, status = %d",
                    iFuseFd->iRodsPath, fd);
                iFuseConnUnlock(iFuseFd->conn);
                pthread_mutex_unlock(&iFuseFd->lock);
                return -ENOENT;
            } else {
                fd = iFuseRodsClientDataObjOpen(iFuseFd->conn->conn, &dataObjOpenInp);
                if (fd <= 0) {
                    iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdReopen: iFuseRodsClientDataObjOpen of %s error, status = %d",
                        iFuseFd->iRodsPath, fd);
                    iFuseConnUnlock(iFuseFd->conn);
                    pthread_mutex_unlock(&iFuseFd->lock);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, fd, "iFuseFdReopen: iFuseRodsClientDataObjOpen of %s error, status = %d",
                iFuseFd->iRodsPath, fd);
            iFuseConnUnlock(iFuseFd->conn);
            pthread_mutex_unlock(&iFuseFd->lock);
            return -ENOENT;
        }
    }
    
    iFuseFd->fd = fd;
    
    iFuseConnUnlock(iFuseFd->conn);
    
    pthread_mutex_unlock(&iFuseFd->lock);
    return status;
}

/*
 * Open a new directory descriptor
 */
int iFuseDirOpen(iFuseDir_t **iFuseDir, iFuseConn_t *iFuseConn, const char* iRodsPath) {
    int status = 0;
    collHandle_t collHandle;
    iFuseDir_t *tmpIFuseDesc;

    assert(iFuseDir != NULL);
    assert(iFuseConn != NULL);
    assert(iRodsPath != NULL);
    
    *iFuseDir = NULL;

    pthread_mutex_lock(&g_AssignedDirLock);
    iFuseConnLock(iFuseConn);
    
    bzero(&collHandle, sizeof ( collHandle_t));
    
    assert(iFuseConn->conn != NULL);
    
    status = iFuseRodsClientOpenCollection(iFuseConn->conn, (char*) iRodsPath, 0, &collHandle);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            // reconnect and retry 
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseDirOpen: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                pthread_mutex_unlock(&g_AssignedDirLock);
                return -ENOENT;
            } else {
                status = iFuseRodsClientOpenCollection(iFuseConn->conn, (char*) iRodsPath, 0, &collHandle);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseDirOpen: iFuseRodsClientOpenCollection of %s error, status = %d",
                        iRodsPath, status);
                    iFuseConnUnlock(iFuseConn);
                    pthread_mutex_unlock(&g_AssignedDirLock);
                    return -ENOENT;
                }
            }
        }
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFdOpen: iFuseRodsClientOpenCollection of %s error, status = %d",
                iRodsPath, status);
            iFuseConnUnlock(iFuseConn);
            pthread_mutex_unlock(&g_AssignedDirLock);
            return -ENOENT;
        }
    }
    
    iFuseConnUnlock(iFuseConn);
    
    tmpIFuseDesc = (iFuseDir_t *) calloc(1, sizeof ( iFuseDir_t));
    if (tmpIFuseDesc == NULL) {
        *iFuseDir = NULL;
        pthread_mutex_unlock(&g_AssignedDirLock);
        return SYS_MALLOC_ERR;
    }
    
    tmpIFuseDesc->ddId = _genNextDdID();
    tmpIFuseDesc->conn = iFuseConn;
    tmpIFuseDesc->iRodsPath = strdup(iRodsPath);
    tmpIFuseDesc->handle = (collHandle_t*)calloc(1, sizeof(collHandle_t));
    if (tmpIFuseDesc->handle == NULL) {
        _freeDir(tmpIFuseDesc);
        pthread_mutex_unlock(&g_AssignedDirLock);
        return SYS_MALLOC_ERR;
    }
    
    memcpy(tmpIFuseDesc->handle, &collHandle, sizeof(collHandle_t));
    
    pthread_mutexattr_init(&tmpIFuseDesc->lockAttr);
    pthread_mutexattr_settype(&tmpIFuseDesc->lockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tmpIFuseDesc->lock, &tmpIFuseDesc->lockAttr);
    
    *iFuseDir = tmpIFuseDesc;
    
    g_AssignedDir.push_back(tmpIFuseDesc);
    
    pthread_mutex_unlock(&g_AssignedDirLock);
    return status;
}

/*
 * Close file descriptor
 */
int iFuseFdClose(iFuseFd_t *iFuseFd) {
    int status = 0;
    
    assert(iFuseFd != NULL);
    assert(iFuseFd->conn != NULL);
    assert(iFuseFd->fd > 0);
    
    pthread_mutex_lock(&g_AssignedFdLock);
    
    g_AssignedFd.remove(iFuseFd);
    status = _freeFd(iFuseFd);
    
    pthread_mutex_unlock(&g_AssignedFdLock);
    return status;
}

/*
 * Close directory descriptor
 */
int iFuseDirClose(iFuseDir_t *iFuseDir) {
    int status = 0;
    
    assert(iFuseDir != NULL);
    assert(iFuseDir->conn != NULL);
    assert(iFuseDir->handle != NULL);
    
    pthread_mutex_lock(&g_AssignedDirLock);
    
    g_AssignedDir.remove(iFuseDir);
    status = _freeDir(iFuseDir);
    
    pthread_mutex_unlock(&g_AssignedDirLock);
    return status;
}

/*
 * Lock file descriptor
 */
void iFuseFdLock(iFuseFd_t *iFuseFd) {
    assert(iFuseFd != NULL);
    
    pthread_mutex_lock(&iFuseFd->lock);
}

/*
 * Lock directory descriptor
 */
void iFuseDirLock(iFuseDir_t *iFuseDir) {
    assert(iFuseDir != NULL);
    
    pthread_mutex_lock(&iFuseDir->lock);
}

/*
 * Unlock file descriptor
 */
void iFuseFdUnlock(iFuseFd_t *iFuseFd) {
    assert(iFuseFd != NULL);
    
    pthread_mutex_unlock(&iFuseFd->lock);
}

/*
 * Unlock directory descriptor
 */
void iFuseDirUnlock(iFuseDir_t *iFuseDir) {
    assert(iFuseDir != NULL);
    
    pthread_mutex_unlock(&iFuseDir->lock);
}
