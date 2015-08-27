/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "iFuse.FS.hpp"
#include "iFuse.Lib.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "iFuse.Lib.Fd.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "iFuse.Lib.Util.hpp"
#include "sockComm.h"

#ifdef _MYSQL_ICAT_DRIVER_PATCH_
#else
#warning _MYSQL_ICAT_DRIVER_PATCH_ is not set. This will increase performance in filesystem operations but may make filesystem inconsistent with MYSQL iCAT database driver.
#endif

static int _fillFileStat(struct stat *stbuf, uint mode, rodsLong_t size, uint ctime, uint mtime, uint atime) {
    if ( mode >= 0100 ) {
        stbuf->st_mode = S_IFREG | mode;
    } else {
        stbuf->st_mode = S_IFREG | DEF_FILE_MODE;
    }
    stbuf->st_size = size;

    stbuf->st_blksize = FILE_BLOCK_SIZE;
    stbuf->st_blocks = ( stbuf->st_size / FILE_BLOCK_SIZE ) + 1;

    stbuf->st_nlink = 1;
    stbuf->st_ino = 0;
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

static int _fillDirStat(struct stat *stbuf, uint ctime, uint mtime, uint atime) {
    stbuf->st_mode = S_IFDIR | DEF_DIR_MODE;
    stbuf->st_size = DIR_SIZE;

    stbuf->st_nlink = 2;
    stbuf->st_ino = 0;
    stbuf->st_ctime = ctime;
    stbuf->st_mtime = mtime;
    stbuf->st_atime = atime;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    return 0;
}

/*
 * Initialize buffer cache manager
 */
void iFuseFsInit() {
}

/*
 * Destroy buffer cache manager
 */
void iFuseFsDestroy() {
}

int iFuseFsGetAttr(const char *iRodsPath, struct stat *stbuf) {
    int status = 0;
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = NULL;
    iFuseConn_t *iFuseConn = NULL;

    assert(iRodsPath != NULL);
    assert(stbuf != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsGetAttr: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
#ifdef _MYSQL_ICAT_DRIVER_PATCH_
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_ONETIMEUSE);
#else
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
#endif
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsGetAttr: iFuseConnGetAndUse of %s error", iRodsPath);
        return -EIO;
    }

    bzero(stbuf, sizeof ( struct stat));
    bzero(&dataObjInp, sizeof ( dataObjInp_t));
    rstrcpy(dataObjInp.objPath, iRodsPath, MAX_NAME_LEN);

    iFuseConnLock(iFuseConn);

    status = iFuseRodsClientObjStat(iFuseConn->conn, &dataObjInp, &rodsObjStatOut);
    if (status < 0 && status != USER_FILE_DOES_NOT_EXIST) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsGetAttr: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientObjStat(iFuseConn->conn, &dataObjInp, &rodsObjStatOut);
                if (status < 0 && status != USER_FILE_DOES_NOT_EXIST) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsGetAttr: iFuseRodsClientObjStat of %s error, status = %d",
                        iRodsPath, status);
                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                } else if(status == USER_FILE_DOES_NOT_EXIST) {
                    // file not exists!
                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsGetAttr: iFuseRodsClientObjStat of %s error, status = %d",
                iRodsPath, status);
            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    } else if (status == USER_FILE_DOES_NOT_EXIST) {
        // file not exists!
        iFuseConnUnlock(iFuseConn);
        iFuseConnUnuse(iFuseConn);
        return -ENOENT;
    }

    if(rodsObjStatOut == NULL) {
        iFuseConnUnlock(iFuseConn);
        iFuseConnUnuse(iFuseConn);
        return -ENOENT;
    }

    if (rodsObjStatOut->objType == COLL_OBJ_T) {
        _fillDirStat(stbuf,
                atoi(rodsObjStatOut->createTime),
                atoi(rodsObjStatOut->modifyTime),
                atoi(rodsObjStatOut->modifyTime));

        status = 0;
    } else if (rodsObjStatOut->objType == UNKNOWN_OBJ_T) {
        status = -ENOENT;
    } else {
        _fillFileStat(stbuf,
                rodsObjStatOut->dataMode,
                rodsObjStatOut->objSize,
                atoi(rodsObjStatOut->createTime),
                atoi(rodsObjStatOut->modifyTime),
                atoi(rodsObjStatOut->modifyTime));

        status = 0;
    }

    freeRodsObjStat(rodsObjStatOut);

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return status;
}

int iFuseFsOpen(const char *iRodsPath, iFuseFd_t **iFuseFd, int openFlag) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;

    assert(iRodsPath != NULL);
    assert(iFuseFd != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsOpen: %s, openFlag: 0x%08x", iRodsPath, openFlag);

    // obtain a connection for a file
    // must be released lock after use
    // while the file is opened, connection is in-use status.
#ifdef _MYSQL_ICAT_DRIVER_PATCH_
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_ONETIMEUSE);
#else
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_FILE_IO);
#endif
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsOpen: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    status = iFuseFdOpen(iFuseFd, iFuseConn, iRodsPath, openFlag);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsOpen: iFuseFdOpen of %s error, status = %d",
                iRodsPath, status);
        return -ENOENT;
    }

    return 0;
}

int iFuseFsClose(iFuseFd_t *iFuseFd) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    char *iRodsPath;
    
    assert(iFuseFd != NULL);
    assert(iFuseFd->iRodsPath != NULL);
    assert(iFuseFd->fd > 0);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsClose: %s", iFuseFd->iRodsPath);

    iFuseConn = iFuseFd->conn;
    
    iRodsPath = strdup(iFuseFd->iRodsPath);
    
    status = iFuseFdClose(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsClose: iFuseFdClose of %s error, status = %d",
                iRodsPath, status);
        free(iRodsPath);
        return -ENOENT;
    }

    free(iRodsPath);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsRead(iFuseFd_t *iFuseFd, char *buf, off_t off, size_t size) {
    int status = 0;
    int readError = 0;
    iFuseConn_t *iFuseConn = NULL;
    openedDataObjInp_t dataObjLseekInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    openedDataObjInp_t dataObjReadInp;
    bytesBuf_t dataObjReadOutBBuf;

    assert(iFuseFd != NULL);
    assert(iFuseFd->iRodsPath != NULL);
    assert(iFuseFd->fd > 0);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsRead: %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);

    iFuseConn = iFuseFd->conn;

    iFuseFdLock(iFuseFd);
    iFuseConnLock(iFuseConn);

    if(iFuseFd->lastFilePointer != off) {
        bzero(&dataObjLseekInp, sizeof( openedDataObjInp_t ));

        dataObjLseekInp.l1descInx = iFuseFd->fd;
        dataObjLseekInp.offset = off;
        dataObjLseekInp.whence = SEEK_SET;

        iFuseRodsClientLog(LOG_DEBUG, "iFuseFsRead: iFuseRodsClientDataObjLseek %s, offset: %lld", iFuseFd->iRodsPath, (long long)off);

        status = iFuseRodsClientDataObjLseek(iFuseConn->conn, &dataObjLseekInp, &dataObjLseekOut);
        if (status < 0 || dataObjLseekOut == NULL) {
            if (iFuseRodsClientReadMsgError(status)) {
                if(iFuseConnReconnect(iFuseConn) < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseConnReconnect of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                    
                    iFuseFd->lastFilePointer = -1;
                    
                    iFuseConnUnlock(iFuseConn);
                    iFuseFdUnlock(iFuseFd);
                    return status;
                } else {
                    status = iFuseRodsClientDataObjLseek(iFuseConn->conn, &dataObjLseekInp, &dataObjLseekOut);
                    if (status < 0 || dataObjLseekOut == NULL) {
                        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjLseek of %s error, status = %d",
                            iFuseFd->iRodsPath, status);
                        
                        iFuseFd->lastFilePointer = -1;
                        
                        iFuseConnUnlock(iFuseConn);
                        iFuseFdUnlock(iFuseFd);
                        return status;
                    }
                }
            } else {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjLseek of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
                
                iFuseFd->lastFilePointer = -1;
                
                iFuseConnUnlock(iFuseConn);
                iFuseFdUnlock(iFuseFd);
                return status;
            }
        }

        if(dataObjLseekOut == NULL) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjLseek failed on seek %s error, offset = %lu, requested = %lu",
                    iFuseFd->iRodsPath, dataObjLseekOut->offset, off);
            
            iFuseFd->lastFilePointer = -1;
            
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            return -ENOENT;
        }

        if(dataObjLseekOut->offset != off) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjLseek failed on seek %s error, offset = %lu, requested = %lu",
                    iFuseFd->iRodsPath, dataObjLseekOut->offset, off);
            
            iFuseFd->lastFilePointer = -1;
            
            free(dataObjLseekOut);
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            return -ENOENT;
        }

        free(dataObjLseekOut);
        dataObjLseekOut = NULL;
        
        iFuseFd->lastFilePointer = off;
    }
        
    bzero(&dataObjReadInp, sizeof ( openedDataObjInp_t));
    bzero(&dataObjReadOutBBuf, sizeof ( bytesBuf_t));

    dataObjReadInp.l1descInx = iFuseFd->fd;
    dataObjReadInp.len = size;

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsRead: iFuseRodsClientDataObjRead %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);

    status = iFuseRodsClientDataObjRead(iFuseConn->conn, &dataObjReadInp, &dataObjReadOutBBuf);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseConnReconnect of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                iFuseFdUnlock(iFuseFd);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjRead(iFuseConn->conn, &dataObjReadInp, &dataObjReadOutBBuf);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjRead of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                    readError = getErrno( status );
                    iFuseConnUnlock(iFuseConn);
                    iFuseFdUnlock(iFuseFd);
                    if(readError > 0) {
                        return -readError;
                    } else {
                        return -ENOENT;
                    }
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRead: iFuseRodsClientDataObjRead of %s error, status = %d",
                iFuseFd->iRodsPath, status);
            readError = getErrno( status );
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            if(readError > 0) {
                return -readError;
            } else {
                return -ENOENT;
            }
        }
    }

    assert(size >= (size_t)status);

    memcpy(buf, dataObjReadOutBBuf.buf, status);

    if(dataObjReadOutBBuf.buf != NULL) {
        free(dataObjReadOutBBuf.buf);
        dataObjReadOutBBuf.buf = NULL;
    }
    
    iFuseFd->lastFilePointer += status;

    iFuseConnUnlock(iFuseConn);
    iFuseFdUnlock(iFuseFd);
    return status;
}

int iFuseFsWrite(iFuseFd_t *iFuseFd, const char *buf, off_t off, size_t size) {
    int status = 0;
    int writeError = 0;
    iFuseConn_t *iFuseConn = NULL;
    openedDataObjInp_t dataObjLseekInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;

    assert(iFuseFd != NULL);
    assert(iFuseFd->iRodsPath != NULL);
    assert(iFuseFd->fd > 0);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsWrite: %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);

    iFuseConn = iFuseFd->conn;

    iFuseFdLock(iFuseFd);
    iFuseConnLock(iFuseConn);

    if(iFuseFd->lastFilePointer != off) {
        bzero(&dataObjLseekInp, sizeof( openedDataObjInp_t ));

        dataObjLseekInp.l1descInx = iFuseFd->fd;
        dataObjLseekInp.offset = off;
        dataObjLseekInp.whence = SEEK_SET;

        iFuseRodsClientLog(LOG_DEBUG, "iFuseFsWrite: iFuseRodsClientDataObjLseek %s, offset: %lld", iFuseFd->iRodsPath, (long long)off);

        status = iFuseRodsClientDataObjLseek(iFuseConn->conn, &dataObjLseekInp, &dataObjLseekOut);
        if (status < 0 || dataObjLseekOut == NULL) {
            if (iFuseRodsClientReadMsgError(status)) {
                if(iFuseConnReconnect(iFuseConn) < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseConnReconnect of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                    
                    iFuseFd->lastFilePointer = -1;
                    
                    iFuseConnUnlock(iFuseConn);
                    iFuseFdUnlock(iFuseFd);
                    return status;
                } else {
                    status = iFuseRodsClientDataObjLseek(iFuseConn->conn, &dataObjLseekInp, &dataObjLseekOut);
                    if (status < 0 || dataObjLseekOut == NULL) {
                        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjLseek of %s error, status = %d",
                            iFuseFd->iRodsPath, status);
                        
                        iFuseFd->lastFilePointer = -1;
                        
                        iFuseConnUnlock(iFuseConn);
                        iFuseFdUnlock(iFuseFd);
                        return status;
                    }
                }
            } else {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjLseek of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
                
                iFuseFd->lastFilePointer = -1;
                
                iFuseConnUnlock(iFuseConn);
                iFuseFdUnlock(iFuseFd);
                return status;
            }
        }

        if(dataObjLseekOut == NULL) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjLseek failed on seek %s error, offset = %lu, requested = %lu",
                    iFuseFd->iRodsPath, dataObjLseekOut->offset, off);
            
            iFuseFd->lastFilePointer = -1;
            
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            return -ENOENT;
        }

        if(dataObjLseekOut->offset != off) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjLseek failed on seek %s error, offset = %lu, requested = %lu",
                    iFuseFd->iRodsPath, dataObjLseekOut->offset, off);
            
            iFuseFd->lastFilePointer = -1;
            
            free(dataObjLseekOut);
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            return -ENOENT;
        }

        free(dataObjLseekOut);
        dataObjLseekOut = NULL;
        
        // update
        iFuseFd->lastFilePointer = off;
    }
    
    bzero(&dataObjWriteInp, sizeof ( openedDataObjInp_t));
    bzero(&dataObjWriteInpBBuf, sizeof ( bytesBuf_t));

    dataObjWriteInp.l1descInx = iFuseFd->fd;
    dataObjWriteInp.len = size;

    dataObjWriteInpBBuf.buf = (char*) buf;
    dataObjWriteInpBBuf.len = size;

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsWrite: iFuseRodsClientDataObjWrite %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);

    status = iFuseRodsClientDataObjWrite(iFuseConn->conn, &dataObjWriteInp, &dataObjWriteInpBBuf);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseConnReconnect of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                iFuseFdUnlock(iFuseFd);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjWrite(iFuseConn->conn, &dataObjWriteInp, &dataObjWriteInpBBuf);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjWrite of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                    writeError = getErrno( status );
                    iFuseConnUnlock(iFuseConn);
                    iFuseFdUnlock(iFuseFd);
                    if(writeError > 0) {
                        return -writeError;
                    } else {
                        return -ENOENT;
                    }
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsWrite: iFuseRodsClientDataObjWrite of %s error, status = %d",
                iFuseFd->iRodsPath, status);
            writeError = getErrno( status );
            iFuseConnUnlock(iFuseConn);
            iFuseFdUnlock(iFuseFd);
            if(writeError > 0) {
                return -writeError;
            } else {
                return -ENOENT;
            }
        }
    }

    iFuseFd->lastFilePointer += status;
    
    iFuseConnUnlock(iFuseConn);
    iFuseFdUnlock(iFuseFd);
    return status;
}

int iFuseFsFlush(iFuseFd_t *iFuseFd) {
    int status = 0;
    
    assert(iFuseFd != NULL);
    assert(iFuseFd->iRodsPath != NULL);
    assert(iFuseFd->fd > 0);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsFlush: %s", iFuseFd->iRodsPath);

    status = iFuseFdReopen(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsClose: iFuseFdReopen of %s error, status = %d",
                iFuseFd->iRodsPath, status);
        return -ENOENT;
    }

    return 0;
}

int iFuseFsCreate(const char *iRodsPath, mode_t mode) {
    int status = 0;
    dataObjInp_t dataObjInp;
    openedDataObjInp_t dataObjCloseInp;
    iFuseConn_t *iFuseConn = NULL;
    int fd = 0;

    assert(iRodsPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsCreate: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: iFuseConnGetAndUse of %s error", iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&dataObjInp, sizeof ( dataObjInp_t));
    rstrcpy(dataObjInp.objPath, iRodsPath, MAX_NAME_LEN);
    if ( strlen( iFuseLibGetRodsEnv()->rodsDefResource ) > 0 ) {
        addKeyVal( &dataObjInp.condInput, RESC_NAME_KW, iFuseLibGetRodsEnv()->rodsDefResource );
    }

    addKeyVal( &dataObjInp.condInput, DATA_TYPE_KW, "generic" );
    dataObjInp.createMode = mode;
    dataObjInp.openFlags = O_RDWR;
    dataObjInp.dataSize = -1;

    status = iFuseRodsClientDataObjCreate(iFuseConn->conn, &dataObjInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                clearKeyVal( &dataObjInp.condInput );

                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjCreate(iFuseConn->conn, &dataObjInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: iFuseRodsClientDataObjCreate of %s error, status = %d",
                        iRodsPath, status);
                    clearKeyVal( &dataObjInp.condInput );

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: iFuseRodsClientDataObjCreate of %s error, status = %d",
                iRodsPath, status);
            clearKeyVal( &dataObjInp.condInput );

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    clearKeyVal( &dataObjInp.condInput );

    fd = status;

    // need to close file
    bzero(&dataObjCloseInp, sizeof (openedDataObjInp_t));
    dataObjCloseInp.l1descInx = fd;

    status = iFuseRodsClientDataObjClose(iFuseConn->conn, &dataObjCloseInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            // reconnect and retry
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: iFuseConnReconnect of %s (%d) error",
                    iRodsPath, fd);
                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjClose(iFuseConn->conn, &dataObjCloseInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: close of %s (%d) error",
                        iRodsPath, fd);
                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCreate: close of %s (%d) error",
                iRodsPath, fd);
            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return status;
}

int iFuseFsUnlink(const char *iRodsPath) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    dataObjInp_t dataObjInp;

    assert(iRodsPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsUnlink: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsUnlink: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&dataObjInp, sizeof ( dataObjInp_t));

    rstrcpy(dataObjInp.objPath, iRodsPath, MAX_NAME_LEN);
    addKeyVal(&dataObjInp.condInput, FORCE_FLAG_KW, "");

    status = iFuseRodsClientDataObjUnlink(iFuseConn->conn, &dataObjInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsUnlink: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                clearKeyVal(&dataObjInp.condInput);

                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjUnlink(iFuseConn->conn, &dataObjInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsUnlink: iFuseRodsClientDataObjUnlink of %s error, status = %d",
                        iRodsPath, status);
                    clearKeyVal(&dataObjInp.condInput);

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsUnlink: iFuseRodsClientDataObjUnlink of %s error, status = %d",
                iRodsPath, status);
            clearKeyVal(&dataObjInp.condInput);

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    clearKeyVal(&dataObjInp.condInput);

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsOpenDir(const char *iRodsPath, iFuseDir_t **iFuseDir) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;

    assert(iRodsPath != NULL);
    assert(iFuseDir != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsOpenDir: %s", iRodsPath);

    // obtain a connection for a file
    // must be released lock after use
    // while the file is opened, connection is in-use status.
#ifdef _MYSQL_ICAT_DRIVER_PATCH_
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_ONETIMEUSE);
#else
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_FILE_IO);
#endif
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsOpenDir: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    status = iFuseDirOpen(iFuseDir, iFuseConn, iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsOpenDir: iFuseDirOpen of %s error, status = %d",
                iRodsPath, status);
        iFuseConnUnlock(iFuseConn);
        return -ENOENT;
    }

    return 0;
}

int iFuseFsCloseDir(iFuseDir_t *iFuseDir) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    char *iRodsPath;
    
    assert(iFuseDir != NULL);
    assert(iFuseDir->iRodsPath != NULL);
    assert(iFuseDir->handle != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsCloseDir: %s", iFuseDir->iRodsPath);

    iFuseConn = iFuseDir->conn;
    
    iRodsPath = strdup(iFuseDir->iRodsPath);
    
    status = iFuseDirClose(iFuseDir);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsCloseDir: iFuseDirClose of %s error, status = %d",
                iRodsPath, status);
        free(iRodsPath);
        return -ENOENT;
    }

    free(iRodsPath);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsReadDir(iFuseDir_t *iFuseDir, iFuseDirFiller filler, void *buf, off_t offset) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    collEnt_t collEnt;

    assert(iFuseDir != NULL);
    assert(iFuseDir->iRodsPath != NULL);
    assert(iFuseDir->handle != NULL);

    UNUSED(offset);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsReadDir: %s", iFuseDir->iRodsPath);

    iFuseConn = iFuseDir->conn;

    iFuseDirLock(iFuseDir);
    iFuseConnLock(iFuseConn);

    bzero(&collEnt, sizeof ( collEnt_t));

    while ((status = iFuseRodsClientReadCollection(iFuseConn->conn, iFuseDir->handle, &collEnt)) >= 0) {
        if (collEnt.objType == DATA_OBJ_T) {
            filler(buf, collEnt.dataName, NULL, 0);
        } else if (collEnt.objType == COLL_OBJ_T) {
            char myDir[MAX_NAME_LEN];
            char mySubDir[MAX_NAME_LEN];

            splitPathByKey(collEnt.collName, myDir, MAX_NAME_LEN, mySubDir, MAX_NAME_LEN, '/');
            if (mySubDir[0] != '\0') {
                filler(buf, mySubDir, NULL, 0);
            }
        }
    }

    iFuseConnUnlock(iFuseConn);
    iFuseDirUnlock(iFuseDir);
    return 0;
}

int iFuseFsMakeDir(const char *iRodsPath, mode_t mode) {
    int status = 0;
    collInp_t collCreateInp;
    iFuseConn_t *iFuseConn = NULL;

    assert(iRodsPath != NULL);

    UNUSED(mode);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsMakeDir: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsMakeDir: iFuseConnGetAndUse of %s error", iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&collCreateInp, sizeof ( collInp_t));
    rstrcpy(collCreateInp.collName, iRodsPath, MAX_NAME_LEN);

    status = iFuseRodsClientCollCreate(iFuseConn->conn, &collCreateInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsMakeDir: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientCollCreate(iFuseConn->conn, &collCreateInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsMakeDir: iFuseRodsClientCollCreate of %s error, status = %d",
                        iRodsPath, status);
                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsMakeDir: iFuseRodsClientCollCreate of %s error, status = %d",
                iRodsPath, status);
            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return status;
}

int iFuseFsRemoveDir(const char *iRodsPath) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    collInp_t collInp;

    assert(iRodsPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsRemoveDir: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&collInp, sizeof ( collInp_t));

    rstrcpy(collInp.collName, iRodsPath, MAX_NAME_LEN);
    addKeyVal(&collInp.condInput, FORCE_FLAG_KW, "");

    status = iFuseRodsClientRmColl(iFuseConn->conn, &collInp, 0);
    if (status < 0 && status != CAT_COLLECTION_NOT_EMPTY) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                clearKeyVal(&collInp.condInput);

                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientRmColl(iFuseConn->conn, &collInp, 0);
                if (status < 0 && status != CAT_COLLECTION_NOT_EMPTY) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseRodsClientRmColl of %s error, status = %d",
                        iRodsPath, status);
                    clearKeyVal(&collInp.condInput);

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                } else if (status == CAT_COLLECTION_NOT_EMPTY) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseRodsClientRmColl of %s error, status = %d",
                        iRodsPath, status);
                    clearKeyVal(&collInp.condInput);

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOTEMPTY;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseRodsClientRmColl of %s error, status = %d",
                iRodsPath, status);
            clearKeyVal(&collInp.condInput);

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    } else if (status == CAT_COLLECTION_NOT_EMPTY) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRemoveDir: iFuseRodsClientRmColl of %s error, status = %d",
                iRodsPath, status);
            clearKeyVal(&collInp.condInput);

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOTEMPTY;
    }

    clearKeyVal(&collInp.condInput);

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsRename(const char *iRodsFromPath, const char *iRodsToPath) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    dataObjCopyInp_t dataObjRenameInp;

    assert(iRodsFromPath != NULL);
    assert(iRodsToPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsRename: %s to %s", iRodsFromPath, iRodsToPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRename: iFuseConnGetAndUse of %s to %s error",
                iRodsFromPath, iRodsToPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&dataObjRenameInp, sizeof ( dataObjCopyInp_t));

    rstrcpy(dataObjRenameInp.srcDataObjInp.objPath, iRodsFromPath, MAX_NAME_LEN);
    rstrcpy(dataObjRenameInp.destDataObjInp.objPath, iRodsToPath, MAX_NAME_LEN);
    addKeyVal(&dataObjRenameInp.destDataObjInp.condInput, FORCE_FLAG_KW, "");
    dataObjRenameInp.srcDataObjInp.oprType = dataObjRenameInp.destDataObjInp.oprType = RENAME_UNKNOWN_TYPE;

    status = iFuseRodsClientDataObjRename(iFuseConn->conn, &dataObjRenameInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRename: iFuseConnReconnect of %s to %s error, status = %d",
                    iRodsFromPath, iRodsToPath, status);
                clearKeyVal(&dataObjRenameInp.destDataObjInp.condInput);

                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjRename(iFuseConn->conn, &dataObjRenameInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRename: iFuseRodsClientDataObjRename of %s to %s error, status = %d",
                        iRodsFromPath, iRodsToPath, status);
                    clearKeyVal(&dataObjRenameInp.destDataObjInp.condInput);

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsRename: iFuseRodsClientDataObjRename of %s to %s error, status = %d",
                iRodsFromPath, iRodsToPath, status);
            clearKeyVal(&dataObjRenameInp.destDataObjInp.condInput);

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    clearKeyVal(&dataObjRenameInp.destDataObjInp.condInput);

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsTruncate(const char *iRodsPath, off_t size) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    dataObjInp_t dataObjInp;

    assert(iRodsPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsTruncate: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsTruncate: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&dataObjInp, sizeof ( dataObjInp_t));

    rstrcpy(dataObjInp.objPath, iRodsPath, MAX_NAME_LEN);
    dataObjInp.dataSize = size;

    status = iFuseRodsClientDataObjTruncate(iFuseConn->conn, &dataObjInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsTruncate: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientDataObjTruncate(iFuseConn->conn, &dataObjInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsTruncate: iFuseRodsClientDataObjTruncate of %s error, status = %d",
                        iRodsPath, status);
                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsTruncate: iFuseRodsClientDataObjTruncate of %s error, status = %d",
                iRodsPath, status);
            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return 0;
}

int iFuseFsChmod(const char *iRodsPath, mode_t mode) {
    int status = 0;
    iFuseConn_t *iFuseConn = NULL;
    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    dataObjInfo_t dataObjInfo;
    char dataMode[SHORT_STR_LEN];

    assert(iRodsPath != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseFsChmod: %s", iRodsPath);

    // temporarily obtain a connection
    // must be marked unused and release lock after use
    status = iFuseConnGetAndUse(&iFuseConn, IFUSE_CONN_TYPE_FOR_SHORTOP);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsChmod: iFuseConnGetAndUse of %s error",
                iRodsPath);
        return -EIO;
    }

    iFuseConnLock(iFuseConn);

    bzero(&regParam, sizeof ( keyValPair_t));
    snprintf(dataMode, SHORT_STR_LEN, "%d", mode);
    addKeyVal(&regParam, DATA_MODE_KW, dataMode);
    addKeyVal(&regParam, ALL_KW, "");

    bzero(&dataObjInfo, sizeof ( dataObjInfo_t));
    rstrcpy(dataObjInfo.objPath, iRodsPath, MAX_NAME_LEN);

    bzero(&modDataObjMetaInp, sizeof ( modDataObjMeta_t));
    modDataObjMetaInp.regParam = &regParam;
    modDataObjMetaInp.dataObjInfo = &dataObjInfo;

    status = iFuseRodsClientModDataObjMeta(iFuseConn->conn, &modDataObjMetaInp);
    if (status < 0) {
        if (iFuseRodsClientReadMsgError(status)) {
            if(iFuseConnReconnect(iFuseConn) < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsChmod: iFuseConnReconnect of %s error, status = %d",
                    iRodsPath, status);
                clearKeyVal(&regParam);

                iFuseConnUnlock(iFuseConn);
                iFuseConnUnuse(iFuseConn);
                return -ENOENT;
            } else {
                status = iFuseRodsClientModDataObjMeta(iFuseConn->conn, &modDataObjMetaInp);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsChmod: iFuseRodsClientModDataObjMeta of %s error, status = %d",
                        iRodsPath, status);
                    clearKeyVal(&regParam);

                    iFuseConnUnlock(iFuseConn);
                    iFuseConnUnuse(iFuseConn);
                    return -ENOENT;
                }
            }
        } else {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseFsChmod: iFuseRodsClientModDataObjMeta of %s error, status = %d",
                iRodsPath, status);
            clearKeyVal(&regParam);

            iFuseConnUnlock(iFuseConn);
            iFuseConnUnuse(iFuseConn);
            return -ENOENT;
        }
    }

    clearKeyVal(&regParam);

    iFuseConnUnlock(iFuseConn);
    iFuseConnUnuse(iFuseConn);
    return 0;
}