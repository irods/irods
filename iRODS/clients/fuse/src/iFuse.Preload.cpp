/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is written by Illyoung Choi (iychoi@email.arizona.edu)      ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <map>
#include <cstring>
#include "iFuse.FS.hpp"
#include "iFuse.Lib.hpp"
#include "iFuse.Lib.Fd.hpp"
#include "iFuse.Preload.hpp"
#include "iFuse.BufferedFS.hpp"
#include "iFuse.Lib.Util.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "miscUtil.h"

static pthread_mutexattr_t g_PreloadLockAttr;
static pthread_mutex_t g_PreloadLock;

static std::map<unsigned long, iFusePreload_t*> g_PreloadMap;

static int _newPreloadPBlock(const char *iRodsPath, iFusePreloadPBlock_t **iFusePreloadPBlock) {
    int status = 0;
    iFusePreloadPBlock_t *tmpIFusePreloadPBlock = NULL;
    
    assert(iRodsPath != NULL);
    assert(iFusePreloadPBlock != NULL);
    
    tmpIFusePreloadPBlock = (iFusePreloadPBlock_t *) calloc(1, sizeof ( iFusePreloadPBlock_t));
    if (tmpIFusePreloadPBlock == NULL) {
        *iFusePreloadPBlock = NULL;
        return SYS_MALLOC_ERR;
    }
    
    tmpIFusePreloadPBlock->fd = NULL;
    tmpIFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_INIT;
    
    pthread_mutexattr_init(&tmpIFusePreloadPBlock->lockAttr);
    pthread_mutexattr_settype(&tmpIFusePreloadPBlock->lockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tmpIFusePreloadPBlock->lock, &tmpIFusePreloadPBlock->lockAttr);
    
    *iFusePreloadPBlock = tmpIFusePreloadPBlock;
    return 0;
}

static int _newPreload(iFusePreload_t **iFusePreload) {
    iFusePreload_t *tmpIFusePreload = NULL;
    
    assert(iFusePreload != NULL);
    
    tmpIFusePreload = (iFusePreload_t *) calloc(1, sizeof ( iFusePreload_t));
    if (tmpIFusePreload == NULL) {
        *iFusePreload = NULL;
        return SYS_MALLOC_ERR;
    }
    
    // we must use new keyword instead of calloc since it contains c++ stl list object
    tmpIFusePreload->pblocks = new std::list<iFusePreloadPBlock_t*>();
    if(tmpIFusePreload->pblocks == NULL) {
        *iFusePreload = NULL;
        return SYS_MALLOC_ERR;
    }
    
    pthread_mutexattr_init(&tmpIFusePreload->lockAttr);
    pthread_mutexattr_settype(&tmpIFusePreload->lockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tmpIFusePreload->lock, &tmpIFusePreload->lockAttr);
    
    *iFusePreload = tmpIFusePreload;
    return 0;
}

static int _freePreloadPBlock(iFusePreloadPBlock_t *iFusePreloadPBlock) {
    assert(iFusePreloadPBlock != NULL);
    
    if(iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_RUNNING || 
            iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_INIT ||
            iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED ||
            iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED) {
        if(!iFusePreloadPBlock->threadJoined) {
            pthread_join(iFusePreloadPBlock->thread, NULL);
            
            pthread_mutex_lock(&iFusePreloadPBlock->lock);

            // set to joined
            iFusePreloadPBlock->threadJoined = true;

            pthread_mutex_unlock(&iFusePreloadPBlock->lock);
        }
    }
    
    pthread_mutex_lock(&iFusePreloadPBlock->lock);
    
    if(iFusePreloadPBlock->fd != NULL) {
        iFuseBufferedFsClose(iFusePreloadPBlock->fd);
        iFusePreloadPBlock->fd = NULL;
    }
    
    iFusePreloadPBlock->blockID = 0;
    
    pthread_mutex_unlock(&iFusePreloadPBlock->lock);
    
    pthread_mutex_destroy(&iFusePreloadPBlock->lock);
    pthread_mutexattr_destroy(&iFusePreloadPBlock->lockAttr);
    
    free(iFusePreloadPBlock);
    return 0;
}

static int _freePreload(iFusePreload_t *iFusePreload) {
    iFusePreloadPBlock_t *iFusePreloadPBlock = NULL;
    
    assert(iFusePreload != NULL);
    
    if(iFusePreload->pblocks != NULL) {
        while(!iFusePreload->pblocks->empty()) {
            iFusePreloadPBlock = iFusePreload->pblocks->front();
            iFusePreload->pblocks->pop_front();

            _freePreloadPBlock(iFusePreloadPBlock);
        }
        
        delete iFusePreload->pblocks;
    }
    
    if(iFusePreload->iRodsPath != NULL) {
        free(iFusePreload->iRodsPath);
        iFusePreload->iRodsPath = NULL;
    }
    
    pthread_mutex_destroy(&iFusePreload->lock);
    pthread_mutexattr_destroy(&iFusePreload->lockAttr);
    
    free(iFusePreload);
    return 0;
}

static int _releaseAllPreload() {
    std::map<unsigned long, iFusePreload_t*>::iterator it_preloadmap;
    iFusePreload_t *iFusePreload = NULL;

    pthread_mutex_lock(&g_PreloadLock);
    
    // release all get
    while(!g_PreloadMap.empty()) {
        it_preloadmap = g_PreloadMap.begin();
        if(it_preloadmap != g_PreloadMap.end()) {
            iFusePreload = it_preloadmap->second;
            g_PreloadMap.erase(it_preloadmap);
            
            _freePreload(iFusePreload);
        }
    }
    
    pthread_mutex_unlock(&g_PreloadLock);
    return 0;
}

static void* _preloadTask(void* param) {
    int status = 0;
    iFusePreloadThreadParam_t *iFusePreloadThreadParam;
    iFusePreload_t *iFusePreload;
    iFusePreloadPBlock_t *iFusePreloadPBlock;
    iFuseFd_t *iFuseFd;
    char *blockBuffer = (char*)calloc(1, getBufferCacheBlockSize());
    
    assert(param != NULL);
    
    iFusePreloadThreadParam = (iFusePreloadThreadParam_t*)param;
    iFusePreload = iFusePreloadThreadParam->preload;
    iFusePreloadPBlock = iFusePreloadThreadParam->pblock;
    
    if(blockBuffer == NULL) {
        free(iFusePreloadThreadParam);
        
        pthread_mutex_lock(&iFusePreloadPBlock->lock);
        iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED;
        pthread_mutex_unlock(&iFusePreloadPBlock->lock);
        return NULL;
    }
    
    iFuseRodsClientLog(LOG_DEBUG, "_preloadTask: preloading %s, blockID: %u", iFusePreload->iRodsPath, iFusePreloadPBlock->blockID);
    
    if(iFusePreloadPBlock->fd == NULL) {
        status = iFuseBufferedFsOpen(iFusePreload->iRodsPath, &iFuseFd, O_RDONLY);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "_preloadTask: iFuseBufferedFsOpen of %s error, status = %d",
                    iFusePreload->iRodsPath, status);
            free(blockBuffer);
            free(iFusePreloadThreadParam);
            
            pthread_mutex_lock(&iFusePreloadPBlock->lock);
            iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED;
            pthread_mutex_unlock(&iFusePreloadPBlock->lock);
            return NULL;
        }
        
        pthread_mutex_lock(&iFusePreloadPBlock->lock);
        iFusePreloadPBlock->fd = iFuseFd;
        pthread_mutex_unlock(&iFusePreloadPBlock->lock);
    }
    
    status = iFuseBufferedFsReadBlock(iFusePreloadPBlock->fd, blockBuffer, iFusePreloadPBlock->blockID);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "_preloadTask: iFuseBufferedFsReadBlock of %s error, status = %d",
                iFusePreloadPBlock->fd->iRodsPath, status);
        free(blockBuffer);
        free(iFusePreloadThreadParam);
            
        pthread_mutex_lock(&iFusePreloadPBlock->lock);
        iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED;
        pthread_mutex_unlock(&iFusePreloadPBlock->lock);
        return NULL;
    }
    
    free(blockBuffer);

    pthread_mutex_lock(&iFusePreloadPBlock->lock);
    iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED;
    pthread_mutex_unlock(&iFusePreloadPBlock->lock);
    
    free(iFusePreloadThreadParam);
    
    return NULL;
}

int _startPreload(iFusePreload_t *iFusePreload, unsigned int blockID, iFuseFd_t *iFuseFd) {
    int status = 0;
    iFusePreloadThreadParam_t *iFusePreloadThreadParam;
    iFusePreloadPBlock_t *iFusePreloadPBlock;
    
    assert(iFusePreload != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "_startPreload: preloading %s, blockID: %u", iFusePreload->iRodsPath, blockID);
    
    status = _newPreloadPBlock(iFusePreload->iRodsPath, &iFusePreloadPBlock);
    if(status < 0) {
        return status;
    }
    
    iFusePreloadPBlock->fd = iFuseFd;
    iFusePreloadPBlock->blockID = blockID;
    iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_RUNNING;
    
    iFusePreloadThreadParam = (iFusePreloadThreadParam_t*) calloc(1, sizeof(iFusePreloadThreadParam_t));
    if(iFusePreloadThreadParam == NULL) {
        return SYS_MALLOC_ERR;
    }
    
    iFusePreloadThreadParam->preload = iFusePreload;
    iFusePreloadThreadParam->pblock = iFusePreloadPBlock;
    
    status = pthread_create(&iFusePreloadPBlock->thread, NULL, _preloadTask, (void*)iFusePreloadThreadParam);
    if(status != 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "_startPreload: failed to create a thread for %s of block id %u, status = %d",
                iFusePreload->iRodsPath, blockID, status);
        iFusePreloadPBlock->status = IFUSE_PRELOAD_PBLOCK_STATUS_CREATION_FAILED;
        _freePreloadPBlock(iFusePreloadPBlock);
        free(iFusePreloadThreadParam);
        return -1;
    }

    pthread_mutex_lock(&iFusePreload->lock);
    
    iFusePreload->pblocks->push_back(iFusePreloadPBlock);
    
    pthread_mutex_unlock(&iFusePreload->lock);
    return status;
}

int _readPreload(iFusePreload_t *iFusePreload, char *buf, unsigned int blockID) {
    int status = 0;
    size_t readSize = 0;
    std::list<iFusePreloadPBlock_t*> removeList;
    std::list<iFusePreloadPBlock_t*> recycleList;
    std::list<iFusePreloadPBlock_t*>::iterator it_preloadpblock;
    iFusePreloadPBlock_t *iFusePreloadPBlock = NULL;
    iFuseFd_t *iFuseFd = NULL;
    bool hasBlock = false;
    bool *pblockExistance = (bool*)calloc(IFUSE_PRELOAD_PBLOCK_NUM, sizeof(bool));
    int i;
    
    assert(iFusePreload != NULL);
    assert(buf != NULL);

    if(pblockExistance == NULL) {
        return SYS_MALLOC_ERR;
    }
    
    bzero(pblockExistance, IFUSE_PRELOAD_PBLOCK_NUM * sizeof(bool));
    
    pthread_mutex_lock(&iFusePreload->lock);
    
    // check loaded
    for(it_preloadpblock=iFusePreload->pblocks->begin();it_preloadpblock!=iFusePreload->pblocks->end();it_preloadpblock++) {
        iFusePreloadPBlock = *it_preloadpblock;

        if(blockID == iFusePreloadPBlock->blockID) {
            // has block
            hasBlock = true;
        } else if(blockID > iFusePreloadPBlock->blockID ||
                blockID + IFUSE_PRELOAD_PBLOCK_NUM < iFusePreloadPBlock->blockID) {
            // remove old blocks
            // if block id is less than current block id
            // or block id is far larger than current block id (for backward read)
            
            iFuseRodsClientLog(LOG_DEBUG, "_readPreload: found old preloaded data of %s, blockID: %u, cur blockID: %u", iFusePreload->iRodsPath, iFusePreloadPBlock->blockID, blockID);
            removeList.push_back(iFusePreloadPBlock);
        } else {
            // preloaded blocks
            if(iFusePreloadPBlock->blockID - blockID - 1 < IFUSE_PRELOAD_PBLOCK_NUM) {
                pblockExistance[iFusePreloadPBlock->blockID - blockID - 1] = true;
            }
        }
    }
    
    // wait old blocks to be joined
    for(it_preloadpblock=removeList.begin();it_preloadpblock!=removeList.end();it_preloadpblock++) {
        iFusePreloadPBlock = *it_preloadpblock;
        
        if(iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_RUNNING || 
                iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_INIT ||
                iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED ||
                iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED) {
            if(!iFusePreloadPBlock->threadJoined) {
                iFuseRodsClientLog(LOG_DEBUG, "_readPreload: waiting for a preload thread of %s, blockID: %u", iFusePreload->iRodsPath, iFusePreloadPBlock->blockID);

                pthread_join(iFusePreloadPBlock->thread, NULL);

                pthread_mutex_lock(&iFusePreloadPBlock->lock);

                // set to joined
                iFusePreloadPBlock->threadJoined = true;

                pthread_mutex_unlock(&iFusePreloadPBlock->lock);
            }
        }
    }
    
    // find reusable pblocks -> moves to recycleList
    // release old pblocks
    while(!removeList.empty()) {
        iFusePreloadPBlock = removeList.front();
        
        iFuseRodsClientLog(LOG_DEBUG, "_readPreload: removing preloaded data of %s, blockID: %u", iFusePreload->iRodsPath, iFusePreloadPBlock->blockID);
        
        removeList.pop_front();
        iFusePreload->pblocks->remove(iFusePreloadPBlock);
        if(iFusePreloadPBlock->fd != NULL &&
                iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED &&
                iFusePreloadPBlock->threadJoined) {
            // reusable
            recycleList.push_back(iFusePreloadPBlock);
        } else {
            _freePreloadPBlock(iFusePreloadPBlock);
        }
    }
    
    if(!hasBlock) {
        iFuseFd = NULL;
        if(!recycleList.empty()) {
            iFusePreloadPBlock = recycleList.front();
            recycleList.pop_front();
            
            iFuseFd = iFusePreloadPBlock->fd;
            iFusePreloadPBlock->fd = NULL;

            _freePreloadPBlock(iFusePreloadPBlock);
        }

        // param iFuseFd can be null
        _startPreload(iFusePreload, blockID, iFuseFd);
    }
    
    for(it_preloadpblock=iFusePreload->pblocks->begin();it_preloadpblock!=iFusePreload->pblocks->end();it_preloadpblock++) {
        iFusePreloadPBlock = *it_preloadpblock;

        if(blockID == iFusePreloadPBlock->blockID) {
            
            if(iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_INIT ||
                    iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_RUNNING || 
                    iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED ||
                    iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED) {
                if(!iFusePreloadPBlock->threadJoined) {
                    iFuseRodsClientLog(LOG_DEBUG, "_readPreload: waiting for a preload thread of %s, blockID: %u", iFusePreload->iRodsPath, blockID);

                    pthread_join(iFusePreloadPBlock->thread, NULL);

                    pthread_mutex_lock(&iFusePreloadPBlock->lock);

                    // set to joined
                    iFusePreloadPBlock->threadJoined = true;

                    pthread_mutex_unlock(&iFusePreloadPBlock->lock);
                }
            }
            
            if(iFusePreloadPBlock->status == IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED &&
                    iFusePreloadPBlock->threadJoined) {
                pthread_mutex_lock(&iFusePreloadPBlock->lock);

                if(iFusePreloadPBlock->fd != NULL) {
                    iFuseRodsClientLog(LOG_DEBUG, "_readPreload: reading a block from preloaded data of %s, blockID: %u", iFusePreload->iRodsPath, blockID);
                    readSize = iFuseBufferedFsReadBlock(iFusePreloadPBlock->fd, buf, blockID);
                } else {
                    readSize = -1;
                }

                pthread_mutex_unlock(&iFusePreloadPBlock->lock);
            } else {
                readSize = -1;
            }
        }
    }
    
    for(i=0;i<IFUSE_PRELOAD_PBLOCK_NUM;i++) {
        if(!pblockExistance[i]) {
            // start preload
            iFuseFd = NULL;
            if(!recycleList.empty()) {
                iFusePreloadPBlock = recycleList.front();
                recycleList.pop_front();
                
                iFuseFd = iFusePreloadPBlock->fd;
                iFusePreloadPBlock->fd = NULL;
                
                _freePreloadPBlock(iFusePreloadPBlock);
            }
            
            // param iFuseFd can be null
            _startPreload(iFusePreload, i + blockID + 1, iFuseFd);
        }
    }
    
    // release entries in recycleList that will not be used
    while(!recycleList.empty()) {
        iFusePreloadPBlock = recycleList.front();
        removeList.pop_front();
        _freePreloadPBlock(iFusePreloadPBlock);
    }
    
    free(pblockExistance);
    pthread_mutex_unlock(&iFusePreload->lock);
    return readSize;
}

/*
 * Initialize preload manager
 */
void iFusePreloadInit() {
    pthread_mutexattr_init(&g_PreloadLockAttr);
    pthread_mutexattr_settype(&g_PreloadLockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_PreloadLock, &g_PreloadLockAttr);
}

/*
 * Destroy preload manager
 */
void iFusePreloadDestroy() {
    _releaseAllPreload();
    
    pthread_mutex_destroy(&g_PreloadLock);
    pthread_mutexattr_destroy(&g_PreloadLockAttr);
}

/*
 * Create a new preload
 */
int iFusePreloadOpen(const char *iRodsPath, iFuseFd_t **iFuseFd, int openFlag) {
    int status = 0;
    std::map<unsigned long, iFusePreload_t*>::iterator it_preloadmap;
    iFusePreload_t *iFusePreload = NULL;
    int i;
    
    assert(iRodsPath != NULL);
    assert(iFuseFd != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFusePreloadOpen: %s, openFlag: 0x%08x", iRodsPath, openFlag);
    
    status = iFuseBufferedFsOpen(iRodsPath, iFuseFd, openFlag);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFusePreloadOpen: iFuseBufferedFsOpen of %s error, status = %d",
                iRodsPath, status);
        return status;
    }
    
    status = _newPreload(&iFusePreload);
    if (status == 0) {
        iFusePreload->fdId = (*iFuseFd)->fdId;
        iFusePreload->iRodsPath = strdup(iRodsPath);

        // start preload thread - only when the file is opened for read
        if((openFlag & O_ACCMODE) == O_RDONLY || (openFlag & O_ACCMODE) == O_RDWR) {
            for(i=0;i<IFUSE_PRELOAD_PBLOCK_NUM;i++) {
                _startPreload(iFusePreload, i, NULL);
            }
        }
        
        pthread_mutex_lock(&g_PreloadLock);

        g_PreloadMap[(*iFuseFd)->fdId] = iFusePreload;

        pthread_mutex_unlock(&g_PreloadLock);
    }
    
    return 0;
}

/*
 * Release a new preload
 */
int iFusePreloadClose(iFuseFd_t *iFuseFd) {
    int status = 0;
    std::map<unsigned long, iFusePreload_t*>::iterator it_preloadmap;
    iFusePreload_t *iFusePreload = NULL;
    char *iRodsPath;
    unsigned long fdId;
    
    assert(iFuseFd != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFusePreloadClose: %s", iFuseFd->iRodsPath);
    
    iRodsPath = strdup(iFuseFd->iRodsPath);
    fdId = iFuseFd->fdId;
    
    status = iFuseBufferedFsClose(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFusePreloadClose: iFuseBufferedFsClose of %s error, status = %d",
                iRodsPath, status);
        free(iRodsPath);
        return -ENOENT;
    }
    
    free(iRodsPath);
    
    pthread_mutex_lock(&g_PreloadLock);
    
    it_preloadmap = g_PreloadMap.find(fdId);
    if(it_preloadmap != g_PreloadMap.end()) {
        // has it
        iFusePreload = it_preloadmap->second;
        g_PreloadMap.erase(it_preloadmap);

        _freePreload(iFusePreload);
    }
    
    pthread_mutex_unlock(&g_PreloadLock);
    
    return status;
}

/*
 * Read preloaded data
 */
int iFusePreloadRead(iFuseFd_t *iFuseFd, char *buf, off_t off, size_t size) {
    int status = 0;
    size_t readSize = 0;
    size_t remain = 0;
    off_t curOffset = 0;
    char *blockBuffer = (char*)calloc(1, getBufferCacheBlockSize());
    std::map<unsigned long, iFusePreload_t*>::iterator it_preloadmap;
    iFusePreload_t *iFusePreload = NULL;
    
    assert(iFuseFd != NULL);
    assert(buf != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFusePreloadRead: %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);
    
    pthread_mutex_lock(&g_PreloadLock);
    
    it_preloadmap = g_PreloadMap.find(iFuseFd->fdId);
    if(it_preloadmap != g_PreloadMap.end()) {
        // has it
        iFusePreload = it_preloadmap->second;
        
        // read in block level
        remain = size;
        curOffset = off;
        while(remain > 0) {
            off_t inBlockOffset = getInBlockOffset(curOffset);
            size_t inBlockAvail = getBufferCacheBlockSize() - inBlockOffset;
            size_t curSize = inBlockAvail > remain ? remain : inBlockAvail;
            size_t blockSize = 0;

            status = _readPreload(iFusePreload, blockBuffer, getBlockID(curOffset));
            if(status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "iFusePreloadRead: _readPreload of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                
                status = iFuseBufferedFsRead(iFuseFd, buf, off, size);
                if (status < 0) {
                    iFuseRodsClientLogError(LOG_ERROR, status, "iFusePreloadRead: iFuseBufferedFsRead of %s error, status = %d",
                            iFuseFd->iRodsPath, status);
                    pthread_mutex_unlock(&g_PreloadLock);
                    free(blockBuffer);
                    return -ENOENT;
                }
                
                pthread_mutex_unlock(&g_PreloadLock);
                free(blockBuffer);
                return status;
            } else if(status == 0) {
                // eof
                break;
            }

            blockSize = (size_t)status;

            if(inBlockOffset + curSize > blockSize) {
                curSize = blockSize - inBlockOffset;
            }

            if(curSize == 0) {
                // eof
                break;
            }

            // copy
            memcpy(buf + readSize, blockBuffer + inBlockOffset, curSize);

            readSize += curSize;
            remain -= curSize;
            curOffset += curSize;

            if(blockSize < getBufferCacheBlockSize()) {
                // eof
                break;
            }
        }
        
        pthread_mutex_unlock(&g_PreloadLock);
        free(blockBuffer);
        return readSize;
    }
    
    // no preloaded data
    pthread_mutex_unlock(&g_PreloadLock);
    
    free(blockBuffer);
    
    status = iFuseBufferedFsRead(iFuseFd, buf, off, size);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFusePreloadRead: iFuseBufferedFsRead of %s error, status = %d",
                iFuseFd->iRodsPath, status);
        return -ENOENT;
    }
    
    return status;
}
