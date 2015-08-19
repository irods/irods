/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is written by Illyoung Choi (iychoi@email.arizona.edu)      ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_PRELOAD_HPP
#define	IFUSE_PRELOAD_HPP

#include <list>
#include <pthread.h>
#include "iFuse.BufferedFS.hpp"
#include "iFuse.Lib.Fd.hpp"

#define IFUSE_PRELOAD_PBLOCK_NUM         3

#define IFUSE_PRELOAD_PBLOCK_STATUS_INIT                 0
#define IFUSE_PRELOAD_PBLOCK_STATUS_RUNNING              1
#define IFUSE_PRELOAD_PBLOCK_STATUS_COMPLETED            2
#define IFUSE_PRELOAD_PBLOCK_STATUS_TASK_FAILED          3
#define IFUSE_PRELOAD_PBLOCK_STATUS_CREATION_FAILED      4

typedef struct IFusePreloadPBlock {
    iFuseFd_t *fd;
    unsigned int blockID;
    int status;
    bool threadJoined;
    pthread_t thread;
    pthread_mutexattr_t lockAttr;
    pthread_mutex_t lock;
} iFusePreloadPBlock_t;

typedef struct IFusePreload {
    unsigned long fdId;
    char *iRodsPath;
    std::list<iFusePreloadPBlock_t*> *pblocks;
    pthread_mutexattr_t lockAttr;
    pthread_mutex_t lock;
} iFusePreload_t;

typedef struct IFusePreloadThreadParam {
    iFusePreload_t *preload;
    iFusePreloadPBlock_t *pblock;
} iFusePreloadThreadParam_t;

void iFusePreloadInit();
void iFusePreloadDestroy();

int iFusePreloadOpen(const char *iRodsPath, iFuseFd_t **iFuseFd, int openFlag);
int iFusePreloadClose(iFuseFd_t *iFuseFd);
int iFusePreloadRead(iFuseFd_t *iFuseFd, char *buf, off_t off, size_t size);

#endif	/* IFUSE_PRELOAD_HPP */

