/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is written by Illyoung Choi (iychoi@email.arizona.edu)      ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_BUFFEREDFS_HPP
#define	IFUSE_BUFFEREDFS_HPP

#include <pthread.h>
#include "iFuse.Lib.Fd.hpp"

#define IFUSE_BUFFER_CACHE_BLOCK_SIZE         (1024*1024*1)

typedef struct IFuseBufferCache {
    unsigned long fdId;
    char *iRodsPath;
    off_t offset;
    size_t size;
    char *buffer;
} iFuseBufferCache_t;

void iFuseBufferedFSInit();
void iFuseBufferedFSDestroy();

size_t getBufferCacheBlockSize();
unsigned int getBlockID(off_t off);
off_t getBlockStartOffset(unsigned int blockID);
off_t getInBlockOffset(off_t off);

int iFuseBufferedFsGetAttr(const char *iRodsPath, struct stat *stbuf);
int iFuseBufferedFsOpen(const char *iRodsPath, iFuseFd_t **iFuseFd, int openFlag);
int iFuseBufferedFsClose(iFuseFd_t *iFuseFd);
int iFuseBufferedFsFlush(iFuseFd_t *iFuseFd);
int iFuseBufferedFsReadBlock(iFuseFd_t *iFuseFd, char *buf, unsigned int blockID);
int iFuseBufferedFsRead(iFuseFd_t *iFuseFd, char *buf, off_t off, size_t size);
int iFuseBufferedFsWrite(iFuseFd_t *iFuseFd, const char *buf, off_t off, size_t size);

#endif	/* IFUSE_BUFFEREDFS_HPP */

