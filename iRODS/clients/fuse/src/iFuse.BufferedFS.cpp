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
#include "iFuse.BufferedFS.hpp"
#include "iFuse.Lib.Util.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "miscUtil.h"

static pthread_mutexattr_t g_BufferCacheLockAttr;
static pthread_mutex_t g_BufferCacheLock;

static std::map<std::string, iFuseBufferCache_t*> g_DeltaMap;
static std::map<unsigned long, iFuseBufferCache_t*> g_CacheMap;

static int _newBufferCache(iFuseBufferCache_t **iFuseBufferCache) {
    iFuseBufferCache_t *tmpIFuseBufferCache = NULL;

    assert(iFuseBufferCache != NULL);

    tmpIFuseBufferCache = (iFuseBufferCache_t *) calloc(1, sizeof ( iFuseBufferCache_t));
    if (tmpIFuseBufferCache == NULL) {
        *iFuseBufferCache = NULL;
        return SYS_MALLOC_ERR;
    }

    *iFuseBufferCache = tmpIFuseBufferCache;
    return 0;
}

static int _freeBufferCache(iFuseBufferCache_t *iFuseBufferCache) {

    assert(iFuseBufferCache != NULL);

    iFuseBufferCache->fdId = 0;

    if(iFuseBufferCache->iRodsPath != NULL) {
        free(iFuseBufferCache->iRodsPath);
        iFuseBufferCache->iRodsPath = NULL;
    }

    if(iFuseBufferCache->buffer != NULL) {
        free(iFuseBufferCache->buffer);
        iFuseBufferCache->buffer = NULL;
    }

    iFuseBufferCache->offset = 0;
    iFuseBufferCache->size = 0;

    free(iFuseBufferCache);
    return 0;
}

size_t getBufferCacheBlockSize() {
    return IFUSE_BUFFER_CACHE_BLOCK_SIZE;
}

unsigned int getBlockID(off_t off) {
    assert(off >= 0);

    return off / IFUSE_BUFFER_CACHE_BLOCK_SIZE;
}

off_t getBlockStartOffset(unsigned int blockID) {
    off_t blockStartOffset = 0;

    blockStartOffset = (off_t)blockID * IFUSE_BUFFER_CACHE_BLOCK_SIZE;

    assert(blockStartOffset >= 0);

    return blockStartOffset;
}

off_t getInBlockOffset(off_t off) {
    off_t inBlockOffset = 0;

    assert(off >= 0);
    
    inBlockOffset = off - getBlockStartOffset(getBlockID(off));
    
    assert(inBlockOffset >= 0);

    return inBlockOffset;
}

static bool _isSameBlock(off_t off1, off_t off2) {
    return getBlockID(off1) == getBlockID(off2);
}

static void _applyDeltaToCache(const char *iRodsPath, const char *buf, off_t off, size_t size) {
    std::map<unsigned long, iFuseBufferCache_t*>::iterator it_cachemap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;

    assert(iRodsPath != NULL);
    assert(buf != NULL);
    assert(off >= 0);
    assert(size > 0);

    pthread_mutex_lock(&g_BufferCacheLock);

    for (it_cachemap = g_CacheMap.begin(); it_cachemap != g_CacheMap.end(); it_cachemap++) {
        iFuseBufferCache = it_cachemap->second;
        
        iFuseRodsClientLog(LOG_DEBUG, "_applyDeltaToCache: comp %s - %s", iFuseBufferCache->iRodsPath, iRodsPath);
        if (strcmp(iFuseBufferCache->iRodsPath, iRodsPath) == 0) {
            if (_isSameBlock(iFuseBufferCache->offset, off)) {
                // update
                off_t endOffset = off + size >  iFuseBufferCache->offset + iFuseBufferCache->size ? off + size : iFuseBufferCache->offset + iFuseBufferCache->size;
                size_t newSize = endOffset - iFuseBufferCache->offset;

                assert(newSize > 0);

                memcpy(iFuseBufferCache->buffer + (off - iFuseBufferCache->offset), buf, size);

                iFuseBufferCache->size = newSize;
            }
        }
    }

    pthread_mutex_unlock(&g_BufferCacheLock);
}

static int _flushDelta(iFuseFd_t *iFuseFd) {
    int status = 0;
    std::map<std::string, iFuseBufferCache_t*>::iterator it_deltamap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;
    std::string pathkey(iFuseFd->iRodsPath);

    assert(iFuseFd != NULL);

    if((iFuseFd->openFlag & O_ACCMODE) != O_RDONLY) {
        pthread_mutex_lock(&g_BufferCacheLock);

        it_deltamap = g_DeltaMap.find(pathkey);
        if(it_deltamap != g_DeltaMap.end()) {
            // has it - flush
            iFuseBufferCache = it_deltamap->second;

            g_DeltaMap.erase(it_deltamap);
            
            // apply to caches
            _applyDeltaToCache(iFuseFd->iRodsPath, iFuseBufferCache->buffer, iFuseBufferCache->offset, iFuseBufferCache->size);
            
            // release lock before making a write request
            pthread_mutex_unlock(&g_BufferCacheLock);
            
            status = iFuseFsWrite(iFuseFd, iFuseBufferCache->buffer, iFuseBufferCache->offset, iFuseBufferCache->size);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "_flushDelta: iFuseFsWrite of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                return -ENOENT;
            }

            // release
            _freeBufferCache(iFuseBufferCache);
        } else {
            pthread_mutex_unlock(&g_BufferCacheLock);
        }
    }

    return 0;
}

static int _readBlock(iFuseFd_t *iFuseFd, char *buf, unsigned int blockID) {
    int status = 0;
    off_t blockStartOffset = 0;
    size_t readSize = 0;
    std::map<unsigned long, iFuseBufferCache_t*>::iterator it_cachemap;
    std::map<std::string, iFuseBufferCache_t*>::iterator it_deltamap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;
    bool hasCache = false;
    std::string pathkey(iFuseFd->iRodsPath);

    assert(iFuseFd != NULL);
    assert(buf != NULL);
    
    blockStartOffset = getBlockStartOffset(blockID);
    
    pthread_mutex_lock(&g_BufferCacheLock);

    // check cache
    it_cachemap = g_CacheMap.find(iFuseFd->fdId);
    if(it_cachemap != g_CacheMap.end()) {
        // has it
        iFuseBufferCache = it_cachemap->second;

        if(iFuseBufferCache->offset == blockStartOffset &&
                iFuseBufferCache->buffer != NULL) {
            memcpy(buf, iFuseBufferCache->buffer, iFuseBufferCache->size);

            readSize = iFuseBufferCache->size;
            hasCache = true;
        } else {
            g_CacheMap.erase(it_cachemap);

            // release
            _freeBufferCache(iFuseBufferCache);
        }
    }
    
    // temporarily unlock mutex
    pthread_mutex_unlock(&g_BufferCacheLock);
    
    if(!hasCache) {
        char *blockBuffer = NULL;
        
        status = _newBufferCache(&iFuseBufferCache);
        if(status < 0) {
            return status;
        }

        assert(iFuseBufferCache != NULL);
        
        // read from server
        blockBuffer = (char*)calloc(1, IFUSE_BUFFER_CACHE_BLOCK_SIZE);
        if(blockBuffer == NULL) {
            _freeBufferCache(iFuseBufferCache);
            return SYS_MALLOC_ERR;
        }
        
        status = iFuseFsRead(iFuseFd, blockBuffer, blockStartOffset, IFUSE_BUFFER_CACHE_BLOCK_SIZE);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "_readBlock: iFuseFsRead of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
            free(blockBuffer);
            _freeBufferCache(iFuseBufferCache);
            return -ENOENT;
        }
        
        iFuseRodsClientLog(LOG_DEBUG, "_readBlock: iFuseFsRead of %s - offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)blockStartOffset, (long long)status);

        iFuseBufferCache->fdId = iFuseFd->fdId;
        iFuseBufferCache->iRodsPath = strdup(iFuseFd->iRodsPath);
        iFuseBufferCache->buffer = blockBuffer;
        iFuseBufferCache->offset = blockStartOffset;
        iFuseBufferCache->size = status;

        pthread_mutex_lock(&g_BufferCacheLock);
        
        g_CacheMap[iFuseFd->fdId] = iFuseBufferCache;

        // copy
        memcpy(buf, iFuseBufferCache->buffer, iFuseBufferCache->size);

        readSize = iFuseBufferCache->size;
        
        pthread_mutex_unlock(&g_BufferCacheLock);
    }
    
    pthread_mutex_lock(&g_BufferCacheLock);
    
    // check delta
    it_deltamap = g_DeltaMap.find(pathkey);
    if(it_deltamap != g_DeltaMap.end()) {
        // has delta
        iFuseBufferCache = it_deltamap->second;
        
        if(getBlockID(iFuseBufferCache->offset) == blockID && 
                iFuseBufferCache->buffer != NULL) {
            size_t deltaSize = 0;

            assert((iFuseBufferCache->offset - blockStartOffset) >= 0);

            memcpy(buf + (iFuseBufferCache->offset - blockStartOffset), iFuseBufferCache->buffer, iFuseBufferCache->size);

            deltaSize = (iFuseBufferCache->offset - blockStartOffset) + iFuseBufferCache->size;

            if(readSize < deltaSize) {
                readSize = deltaSize;
            }
        }
    }

    pthread_mutex_unlock(&g_BufferCacheLock);
    return readSize;
}

static int _writeBlock(iFuseFd_t *iFuseFd, const char *buf, off_t off, size_t size) {
    int status = 0;
    std::map<std::string, iFuseBufferCache_t*>::iterator it_deltamap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;
    std::string pathkey(iFuseFd->iRodsPath);

    assert(iFuseFd != NULL);
    assert(buf != NULL);
    assert(size > 0);

    pthread_mutex_lock(&g_BufferCacheLock);

    it_deltamap = g_DeltaMap.find(pathkey);
    if(it_deltamap != g_DeltaMap.end()) {
        // has it - determine flush or extend
        iFuseBufferCache = it_deltamap->second;

        if(_isSameBlock(iFuseBufferCache->offset, off) &&
            (off_t)(iFuseBufferCache->offset + iFuseBufferCache->size) >= off &&
            iFuseBufferCache->offset <= (off_t)(off + size)) {
            // intersect - expand
            off_t startOffset = off > iFuseBufferCache->offset ? iFuseBufferCache->offset : off;
            off_t endOffset = (off + size) > (iFuseBufferCache->offset + iFuseBufferCache->size) ? (off + size) : (iFuseBufferCache->offset + iFuseBufferCache->size);
            size_t newSize = endOffset - startOffset;
            char *newBuf = (char*)calloc(1, newSize);
            if(newBuf == NULL) {
                pthread_mutex_unlock(&g_BufferCacheLock);
                return SYS_MALLOC_ERR;
            }

            assert(newSize > 0);
            assert((iFuseBufferCache->offset - startOffset) >= 0);
            assert((off - startOffset) >= 0);

            memcpy(newBuf + (iFuseBufferCache->offset - startOffset), iFuseBufferCache->buffer, iFuseBufferCache->size);
            memcpy(newBuf + (off - startOffset), buf, size);

            free(iFuseBufferCache->buffer);

            iFuseBufferCache->buffer = newBuf;
            iFuseBufferCache->offset = startOffset;
            iFuseBufferCache->size = newSize;
            
            pthread_mutex_unlock(&g_BufferCacheLock);
        } else {
            char *newBuf = (char*)calloc(1, size);
            char *bufFlush = iFuseBufferCache->buffer;
            off_t offFlush = iFuseBufferCache->offset;
            size_t sizeFlush = iFuseBufferCache->size;
            
            if(newBuf == NULL) {
                pthread_mutex_unlock(&g_BufferCacheLock);
                return SYS_MALLOC_ERR;
            }
            
            // discrete
            // apply delta to caches
            _applyDeltaToCache(iFuseFd->iRodsPath, iFuseBufferCache->buffer, iFuseBufferCache->offset, iFuseBufferCache->size);
            
            memcpy(newBuf, buf, size);
            
            iFuseBufferCache->buffer = newBuf;
            iFuseBufferCache->offset = off;
            iFuseBufferCache->size = size;
            
            // release mutex before making write request
            pthread_mutex_unlock(&g_BufferCacheLock);
            
            // flush
            status = iFuseFsWrite(iFuseFd, bufFlush, offFlush, sizeFlush);
            if (status < 0) {
                iFuseRodsClientLogError(LOG_ERROR, status, "_writeBlock: iFuseFsWrite of %s error, status = %d",
                        iFuseFd->iRodsPath, status);
                return -ENOENT;
            }

            free(bufFlush);
        }
    } else {
        char *newBuf = (char*)calloc(1, size);
        if(newBuf == NULL) {
            pthread_mutex_unlock(&g_BufferCacheLock);
            return SYS_MALLOC_ERR;
        }
        
        // no delta
        status = _newBufferCache(&iFuseBufferCache);
        if(status < 0) {
            pthread_mutex_unlock(&g_BufferCacheLock);
            return status;
        }

        assert(iFuseBufferCache != NULL);

        memcpy(newBuf, buf, size);

        iFuseBufferCache->fdId = iFuseFd->fdId;
        iFuseBufferCache->iRodsPath = strdup(iFuseFd->iRodsPath);
        iFuseBufferCache->buffer = newBuf;
        iFuseBufferCache->offset = off;
        iFuseBufferCache->size = size;

        g_DeltaMap[pathkey] = iFuseBufferCache;
        
        pthread_mutex_unlock(&g_BufferCacheLock);
    }
    
    return 0;
}

static int _releaseAllCache() {
    std::map<std::string, iFuseBufferCache_t*>::iterator it_deltamap;
    std::map<unsigned long, iFuseBufferCache_t*>::iterator it_cachemap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;

    pthread_mutex_lock(&g_BufferCacheLock);

    // release all caches
    while(!g_DeltaMap.empty()) {
        it_deltamap = g_DeltaMap.begin();
        if(it_deltamap != g_DeltaMap.end()) {
            iFuseBufferCache = it_deltamap->second;
            g_DeltaMap.erase(it_deltamap);

            _freeBufferCache(iFuseBufferCache);
        }
    }

    while(!g_CacheMap.empty()) {
        it_cachemap = g_CacheMap.begin();
        if(it_cachemap != g_CacheMap.end()) {
            iFuseBufferCache = it_cachemap->second;
            g_CacheMap.erase(it_cachemap);

            _freeBufferCache(iFuseBufferCache);
        }
    }

    pthread_mutex_unlock(&g_BufferCacheLock);
    return 0;
}

/*
 * Initialize buffer cache manager
 */
void iFuseBufferedFSInit() {
    pthread_mutexattr_init(&g_BufferCacheLockAttr);
    pthread_mutexattr_settype(&g_BufferCacheLockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_BufferCacheLock, &g_BufferCacheLockAttr);
}

/*
 * Destroy buffer cache manager
 */
void iFuseBufferedFSDestroy() {
    _releaseAllCache();

    pthread_mutex_destroy(&g_BufferCacheLock);
    pthread_mutexattr_destroy(&g_BufferCacheLockAttr);
}

int iFuseBufferedFsGetAttr(const char *iRodsPath, struct stat *stbuf) {
    int status = 0;
    std::map<std::string, iFuseBufferCache_t*>::iterator it_deltamap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;
    std::string pathkey(iRodsPath);

    assert(iRodsPath != NULL);
    assert(stbuf != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsGetAttr: %s", iRodsPath);

    status = iFuseFsGetAttr(iRodsPath, stbuf);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsGetAttr: iFuseFsGetAttr of %s error, status = %d",
                iRodsPath, status);
        return status;
    }

    pthread_mutex_lock(&g_BufferCacheLock);

    it_deltamap = g_DeltaMap.find(pathkey);
    if(it_deltamap != g_DeltaMap.end()) {
        // has it
        iFuseBufferCache = it_deltamap->second;

        off_t newSize = iFuseBufferCache->offset + iFuseBufferCache->size;
        if(newSize > stbuf->st_size) {
            stbuf->st_size = newSize;
        }
    }

    pthread_mutex_unlock(&g_BufferCacheLock);
    return status;
}

/*
 * Create a new buffer cache
 */
int iFuseBufferedFsOpen(const char *iRodsPath, iFuseFd_t **iFuseFd, int openFlag) {
    int status = 0;

    assert(iRodsPath != NULL);
    assert(iFuseFd != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsOpen: %s, openFlag: 0x%08x", iRodsPath, openFlag);

    status = iFuseFsOpen(iRodsPath, iFuseFd, openFlag);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsOpen: iFuseFsOpen of %s error, status = %d",
                iRodsPath, status);
        return status;
    }

    return status;
}

/*
 * Release a buffer cache
 */
int iFuseBufferedFsClose(iFuseFd_t *iFuseFd) {
    int status = 0;
    std::map<unsigned long, iFuseBufferCache_t*>::iterator it_cachemap;
    iFuseBufferCache_t *iFuseBufferCache = NULL;
    char *iRodsPath;
    
    assert(iFuseFd != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsClose: %s", iFuseFd->iRodsPath);
    
    if((iFuseFd->openFlag & O_ACCMODE) != O_RDONLY) {
        // flush if necessary
        status = _flushDelta(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsClose: _flushCache of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
            return -ENOENT;
        }
    }
    
    pthread_mutex_lock(&g_BufferCacheLock);
    
    it_cachemap = g_CacheMap.find(iFuseFd->fdId);
    if(it_cachemap != g_CacheMap.end()) {
        // has it
        iFuseBufferCache = it_cachemap->second;
        g_CacheMap.erase(it_cachemap);

        _freeBufferCache(iFuseBufferCache);
    }

    pthread_mutex_unlock(&g_BufferCacheLock);
    
    iRodsPath = strdup(iFuseFd->iRodsPath);
    
    status = iFuseFsClose(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsClose: iFuseFsClose of %s error, status = %d",
                iRodsPath, status);
        free(iRodsPath);
        return -ENOENT;
    }
    
    free(iRodsPath);
    return status;
}

/*
 * Flush buffer cache data
 */
int iFuseBufferedFsFlush(iFuseFd_t *iFuseFd) {
    int status = 0;

    assert(iFuseFd != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsFlush: %s", iFuseFd->iRodsPath);
    
    if((iFuseFd->openFlag & O_ACCMODE) != O_RDONLY) {
        status = _flushDelta(iFuseFd);
        if (status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsFlush: _flushCache of %s error, status = %d",
                    iFuseFd->iRodsPath, status);
            return -ENOENT;
        }
    }
    
    status = iFuseFsFlush(iFuseFd);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsFlush: iFuseFsFlush of %s error, status = %d",
                iFuseFd->iRodsPath, status);
        return -ENOENT;
    }
    
    return status;
}

int iFuseBufferedFsReadBlock(iFuseFd_t *iFuseFd, char *buf, unsigned int blockID) {
    int status = 0;
    
    assert(iFuseFd != NULL);
    assert(buf != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsReadBlock: %s, blockID: %u", iFuseFd->iRodsPath, blockID);
    
    status = _readBlock(iFuseFd, buf, blockID);
    if(status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsReadBlock: _readBlock of %s error, status = %d",
            iFuseFd->iRodsPath, status);
        return status;
    }
        
    return status;
}

/*
 * Read buffer cache data
 */
int iFuseBufferedFsRead(iFuseFd_t *iFuseFd, char *buf, off_t off, size_t size) {
    int status = 0;
    size_t readSize = 0;
    size_t remain = 0;
    off_t curOffset = 0;
    char *blockBuffer = (char*)calloc(1, IFUSE_BUFFER_CACHE_BLOCK_SIZE);
    if(blockBuffer == NULL) {
        return SYS_MALLOC_ERR;
    }

    assert(iFuseFd != NULL);
    assert(buf != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsRead: %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);
    
    // read in block level
    remain = size;
    curOffset = off;
    while(remain > 0) {
        off_t inBlockOffset = getInBlockOffset(curOffset);
        size_t inBlockAvail = IFUSE_BUFFER_CACHE_BLOCK_SIZE - inBlockOffset;
        size_t curSize = inBlockAvail > remain ? remain : inBlockAvail;
        size_t blockSize = 0;
        
        status = _readBlock(iFuseFd, blockBuffer, getBlockID(curOffset));
        if(status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsRead: _readBlock of %s error, status = %d",
                iFuseFd->iRodsPath, status);
            return status;
        } else if(status == 0) {
            // eof
            break;
        }

        blockSize = (size_t)status;

        iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsRead: _readBlock of %s, offset: %lld, in-block offset: %lld, curSize: %lld, size: %lld", iFuseFd->iRodsPath, (long long)curOffset, (long long)inBlockOffset, (long long)curSize, (long long)blockSize);

        if(inBlockOffset + curSize > blockSize) {
            curSize = blockSize - inBlockOffset;
        }

        if (curSize == 0) {
            // eof
            break;
        }

        iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsRead: block read of %s - offset %lld, size %lld", iFuseFd->iRodsPath, (long long)curOffset, (long long)curSize);

        // copy
        memcpy(buf + readSize, blockBuffer + inBlockOffset, curSize);

        readSize += curSize;
        remain -= curSize;
        curOffset += curSize;

        if(blockSize < IFUSE_BUFFER_CACHE_BLOCK_SIZE) {
            // eof
            break;
        }
    }

    free(blockBuffer);
    
    return readSize;
}

/*
 * Write buffer cache data
 */
int iFuseBufferedFsWrite(iFuseFd_t *iFuseFd, const char *buf, off_t off, size_t size) {
    int status = 0;
    size_t writtenSize = 0;
    size_t remain = 0;
    off_t curOffset = 0;

    assert(iFuseFd != NULL);
    assert(buf != NULL);

    iFuseRodsClientLog(LOG_DEBUG, "iFuseBufferedFsWrite: %s, offset: %lld, size: %lld", iFuseFd->iRodsPath, (long long)off, (long long)size);
    
    // write in block level
    remain = size;
    curOffset = off;
    while(remain > 0) {
        off_t inBlockOffset = getInBlockOffset(curOffset);
        size_t inBlockAvail = IFUSE_BUFFER_CACHE_BLOCK_SIZE - inBlockOffset;
        size_t curSize = inBlockAvail > remain ? remain : inBlockAvail;

        assert(curSize > 0);

        status = _writeBlock(iFuseFd, buf + writtenSize, curOffset, curSize);
        if(status < 0) {
            iFuseRodsClientLogError(LOG_ERROR, status, "iFuseBufferedFsWrite: _writeBlock of %s error, status = %d",
                iFuseFd->iRodsPath, status);
            return status;
        }

        writtenSize += curSize;
        remain -= curSize;
        curOffset += curSize;
    }
    
    return writtenSize;
}
