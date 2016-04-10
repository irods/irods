/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_LIB_CONN_HPP
#define	IFUSE_LIB_CONN_HPP

#include <pthread.h>
#include "rodsClient.h"

#define IFUSE_MAX_NUM_CONN	10

#define IFUSE_CONN_TYPE_FOR_FILE_IO      0
#define IFUSE_CONN_TYPE_FOR_SHORTOP      1
#define IFUSE_CONN_TYPE_FOR_ONETIMEUSE   2

#define IFUSE_FREE_CONN_CHECK_PERIOD    10
#define IFUSE_FREE_CONN_TIMEOUT_SEC     (60*5)
#define IFUSE_FREE_CONN_KEEPALIVE_SEC   (60*3)

typedef struct IFuseConn {
    unsigned long connId;
    int type;
    rcComm_t *conn;
    time_t actTime;
    time_t lastKeepAliveTime;
    int inuseCnt;
    pthread_mutexattr_t lockAttr;
    pthread_mutex_t lock;
} iFuseConn_t;

/*
 * Usage pattern
 * - iFuseConnInit
 * - iFuseConnGetAndUse
 * - iFuseConnLock
 * - some operations
 * - iFuseConnUnlock
 * - iFuseConnUnuse
 * - iFuseConnDestroy
 */

int iFuseConnTest();
void iFuseConnInit();
void iFuseConnDestroy();
int iFuseConnGetAndUse(iFuseConn_t **iFuseConn, int connType);
int iFuseConnUnuse(iFuseConn_t *iFuseConn);
int iFuseConnReconnect(iFuseConn_t *iFuseConn);
void iFuseConnLock(iFuseConn_t *iFuseConn);
void iFuseConnUnlock(iFuseConn_t *iFuseConn);

#endif	/* IFUSE_LIB_CONN_HPP */
