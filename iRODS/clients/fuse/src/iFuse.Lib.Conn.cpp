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
#include "iFuse.Lib.Conn.hpp"
#include "iFuse.Lib.Util.hpp"

static pthread_mutex_t g_ConnectedConnLock;
static pthread_mutexattr_t g_ConnectedConnLockAttr;
static iFuseConn_t* g_InUseStatConn;
static iFuseConn_t** g_InUseConn;
static iFuseConn_t* g_FreeStatConn;
static std::list<iFuseConn_t*> g_FreeConn;

static pthread_t g_FreeConnCollector;
static bool g_FreeConnCollectorRunning;
static unsigned long g_connIDGen;

static int g_maxConnNum = IFUSE_MAX_NUM_CONN;
static int g_connTimeoutSec = IFUSE_FREE_CONN_TIMEOUT_SEC;
static int g_connKeepAliveSec = IFUSE_FREE_CONN_KEEPALIVE_SEC;

/*
 * Lock order : 
 * - g_ConnectedConnLock
 * - iFuseConn_t
 */

static unsigned long _genNextConnID() {
    return g_connIDGen++;
}

static int _connect(iFuseConn_t *iFuseConn) {
    int status = 0;
    rodsEnv *myRodsEnv = iFuseLibGetRodsEnv();
    int reconnFlag = NO_RECONN;
    
    assert(myRodsEnv != NULL);
    assert(iFuseConn != NULL);
    
    pthread_mutex_lock(&iFuseConn->lock);

    if (iFuseConn->conn == NULL) {
        rErrMsg_t errMsg;
        iFuseConn->conn = iFuseRodsClientConnect(myRodsEnv->rodsHost, myRodsEnv->rodsPort,
                myRodsEnv->rodsUserName, myRodsEnv->rodsZone, reconnFlag, &errMsg);
        if (iFuseConn->conn == NULL) {
            // try one more
            iFuseConn->conn = iFuseRodsClientConnect(myRodsEnv->rodsHost, myRodsEnv->rodsPort,
                    myRodsEnv->rodsUserName, myRodsEnv->rodsZone, reconnFlag, &errMsg);
            if (iFuseConn->conn == NULL) {
                // failed
                iFuseRodsClientLogError(LOG_ERROR, errMsg.status,
                        "_connect: iFuseRodsClientConnect failure %s", errMsg.msg);

                pthread_mutex_unlock(&iFuseConn->lock);
                if (errMsg.status < 0) {
                    return errMsg.status;
                } else {
                    return -1;
                }
            }
        }

        assert(iFuseConn->conn != NULL);
        
        status = iFuseRodsClientLogin(iFuseConn->conn);
        if (status != 0) {
            iFuseRodsClientDisconnect(iFuseConn->conn);
            iFuseConn->conn = NULL;
            
            // failed
            iFuseRodsClientLog(LOG_ERROR, "iFuseRodsClientLogin failure, status = %d", status);
        }
    }
    
    pthread_mutex_unlock(&iFuseConn->lock);
    return status;
}

static void _disconnect(iFuseConn_t *iFuseConn) {
    assert(iFuseConn != NULL);
    
    pthread_mutex_lock(&iFuseConn->lock);

    if (iFuseConn->conn != NULL) {
        iFuseRodsClientDisconnect(iFuseConn->conn);
        iFuseConn->conn = NULL;
    }

    pthread_mutex_unlock(&iFuseConn->lock);
}

static int _newConn(iFuseConn_t **iFuseConn) {
    int status = 0;
    iFuseConn_t *tmpIFuseConn = NULL;
    
    assert(iFuseConn != NULL);
    
    tmpIFuseConn = (iFuseConn_t *) calloc(1, sizeof ( iFuseConn_t));
    if (tmpIFuseConn == NULL) {
        *iFuseConn = NULL;
        return SYS_MALLOC_ERR;
    }
    
    tmpIFuseConn->connId = _genNextConnID();
    
    iFuseRodsClientLog(LOG_DEBUG, "_newConn: creating a new connection - %lu", tmpIFuseConn->connId);
    
    pthread_mutexattr_init(&tmpIFuseConn->lockAttr);
    pthread_mutexattr_settype(&tmpIFuseConn->lockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tmpIFuseConn->lock, &tmpIFuseConn->lockAttr);

    // connect
    status = _connect(tmpIFuseConn);
    *iFuseConn = tmpIFuseConn;
    return status;
}

static int _freeConn(iFuseConn_t *iFuseConn) {
    assert(iFuseConn != NULL);
    
    iFuseRodsClientLog(LOG_DEBUG, "_freeConn: disconnecting - %lu", iFuseConn->connId);
    
    // disconnect first
    _disconnect(iFuseConn);

    pthread_mutex_destroy(&iFuseConn->lock);
    pthread_mutexattr_destroy(&iFuseConn->lockAttr);

    free(iFuseConn);
    return 0;
}

static int _freeAllConn() {
    iFuseConn_t *tmpIFuseConn;
    int i;

    pthread_mutex_lock(&g_ConnectedConnLock);
    
    // disconnect all free connections
    while(!g_FreeConn.empty()) {
        tmpIFuseConn = g_FreeConn.front();
        g_FreeConn.pop_front();
        
        _freeConn(tmpIFuseConn);
    }
    
    if(g_FreeStatConn != NULL) {
        _freeConn(g_FreeStatConn);
        g_FreeStatConn = NULL;
    }
    
    // disconnect all inuse connections
    for(i=0;i<g_maxConnNum;i++) {
        if(g_InUseConn[i] != NULL) {
            tmpIFuseConn = g_InUseConn[i];
            g_InUseConn[i] = NULL;
            _freeConn(tmpIFuseConn);
        }
    }
    
    if(g_InUseStatConn != NULL) {
        _freeConn(g_InUseStatConn);
        g_InUseStatConn = NULL;
    }
    
    pthread_mutex_unlock(&g_ConnectedConnLock);
    
    return 0;
}

static void _keepAlive(iFuseConn_t *iFuseConn) {
    int status = 0;
    char iRodsPath[MAX_NAME_LEN];
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = NULL;
    
    bzero(iRodsPath, MAX_NAME_LEN);
    status = iFuseRodsClientMakeRodsPath("/", iRodsPath);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status,
                "_keepAlive: iFuseRodsClientMakeRodsPath of %s error", iRodsPath);
        return;
    }
    
    iFuseRodsClientLog(LOG_DEBUG, "_keepAlive: %s", iRodsPath);
    
    bzero(&dataObjInp, sizeof ( dataObjInp_t));
    rstrcpy(dataObjInp.objPath, iRodsPath, MAX_NAME_LEN);
    
    iFuseConnLock(iFuseConn);
    
    status = iFuseRodsClientObjStat(iFuseConn->conn, &dataObjInp, &rodsObjStatOut);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "_keepAlive: iFuseRodsClientObjStat of %s error, status = %d",
            iRodsPath, status);
        iFuseConnUnlock(iFuseConn);
        return;
    }
    
    if(rodsObjStatOut != NULL) {
        freeRodsObjStat(rodsObjStatOut);
    }
    
    iFuseConnUnlock(iFuseConn);
}

static void* _connChecker(void* param) {
    std::list<iFuseConn_t*> removeList;
    std::list<iFuseConn_t*>::iterator it_conn;
    iFuseConn_t *iFuseConn;
    time_t currentTime;
    int i;
    
    UNUSED(param);
    
    iFuseRodsClientLog(LOG_DEBUG, "_connChecker: collector is running");
    
    while(g_FreeConnCollectorRunning) {
        pthread_mutex_lock(&g_ConnectedConnLock);
        
        currentTime = iFuseLibGetCurrentTime();
        
        for(i=0;i<g_maxConnNum;i++) {
            if(g_InUseConn[i] != NULL) {
                if(IFuseLibDiffTimeSec(currentTime, g_InUseConn[i]->lastKeepAliveTime) >= g_connKeepAliveSec) {
                    _keepAlive(g_InUseConn[i]);
                    g_InUseConn[i]->lastKeepAliveTime = currentTime;
                }
            }
        }

        if(g_InUseStatConn != NULL) {
            if(IFuseLibDiffTimeSec(currentTime, g_InUseStatConn->lastKeepAliveTime) >= g_connKeepAliveSec) {
                _keepAlive(g_InUseStatConn);
                g_InUseStatConn->lastKeepAliveTime = currentTime;
            }
        }
        
        //iFuseRodsClientLog(LOG_DEBUG, "_freeConnCollector: checking idle connections");
        
        // iterate free conn list to check timedout connections
        for(it_conn=g_FreeConn.begin();it_conn!=g_FreeConn.end();it_conn++) {
            iFuseConn = *it_conn;
            
            if(IFuseLibDiffTimeSec(currentTime, iFuseConn->actTime) >= g_connTimeoutSec) {
                removeList.push_back(iFuseConn);
            }
        }
        
        // iterate remove list
        while(!removeList.empty()) {
            iFuseConn = removeList.front();
            removeList.pop_front();
            g_FreeConn.remove(iFuseConn);
            _freeConn(iFuseConn);
        }
        
        if(g_FreeStatConn != NULL) {
            if(IFuseLibDiffTimeSec(currentTime, g_FreeStatConn->actTime) >= g_connTimeoutSec) {
                _freeConn(g_FreeStatConn);
                g_FreeStatConn = NULL;
            }
        }
        
        pthread_mutex_unlock(&g_ConnectedConnLock);
        
        sleep(IFUSE_FREE_CONN_CHECK_PERIOD);
    }
    
    return NULL;
}

/*
 * Initialize Conn Manager
 */
void iFuseConnInit() {
    int i;
    
    if(iFuseLibGetOption()->maxConn > 0) {
        g_maxConnNum = iFuseLibGetOption()->maxConn;
    }
    
    if(iFuseLibGetOption()->connTimeoutSec > 0) {
        g_connTimeoutSec = iFuseLibGetOption()->connTimeoutSec;
    }
    
    if(iFuseLibGetOption()->connKeepAliveSec > 0) {
        g_connKeepAliveSec = iFuseLibGetOption()->connKeepAliveSec;
    }
    
    pthread_mutexattr_init(&g_ConnectedConnLockAttr);
    pthread_mutexattr_settype(&g_ConnectedConnLockAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_ConnectedConnLock, &g_ConnectedConnLockAttr);
    
    g_InUseConn = (iFuseConn_t**)calloc(g_maxConnNum, sizeof(iFuseConn_t*));
    
    for(i=0;i<g_maxConnNum;i++) {
        g_InUseConn[i] = NULL;
    }
    
    g_connIDGen = 0;
    
    g_FreeConnCollectorRunning = true;
    
    pthread_create(&g_FreeConnCollector, NULL, _connChecker, NULL);
}

/*
 * Destroy Conn Manager
 */
void iFuseConnDestroy() {
    g_connIDGen = 0;
    
    g_FreeConnCollectorRunning = false;
    
    _freeAllConn();
    
    pthread_join(g_FreeConnCollector, NULL);
    
    pthread_mutex_destroy(&g_ConnectedConnLock);
    pthread_mutexattr_destroy(&g_ConnectedConnLockAttr);
    
    free(g_InUseConn);
}

/*
 * Get connection and increase reference count
 */
int iFuseConnGetAndUse(iFuseConn_t **iFuseConn, int connType) {
    int status;
    iFuseConn_t *tmpIFuseConn;
    int i;
    int targetIndex;
    int inUseCount;

    assert(iFuseConn != NULL);
    
    *iFuseConn = NULL;
    
    pthread_mutex_lock(&g_ConnectedConnLock);
    
    if(connType == IFUSE_CONN_TYPE_FOR_STATUS) {
        if(g_InUseStatConn != NULL) {
            pthread_mutex_lock(&g_InUseStatConn->lock);
            g_InUseStatConn->actTime = iFuseLibGetCurrentTime();
            g_InUseStatConn->inuseCnt++;
            pthread_mutex_unlock(&g_InUseStatConn->lock);
            
            *iFuseConn = g_InUseStatConn;
            pthread_mutex_unlock(&g_ConnectedConnLock);
            return 0;
        }
        
        // not in inusestatconn
        // check free conn
        if (g_FreeStatConn != NULL) {
            // reuse existing connection
            tmpIFuseConn = g_FreeStatConn;
            g_FreeStatConn = NULL;

            pthread_mutex_lock(&tmpIFuseConn->lock);
            tmpIFuseConn->actTime = iFuseLibGetCurrentTime();
            tmpIFuseConn->inuseCnt++;
            pthread_mutex_unlock(&tmpIFuseConn->lock);

            *iFuseConn = tmpIFuseConn;

            g_InUseStatConn = tmpIFuseConn;

            pthread_mutex_unlock(&g_ConnectedConnLock);
            return 0;
        }
        
        // need to create new
        status = _newConn(&tmpIFuseConn);
        if (status < 0) {
            _freeConn(tmpIFuseConn);
            pthread_mutex_unlock(&g_ConnectedConnLock);
            return status;
        }
        
        tmpIFuseConn->actTime = iFuseLibGetCurrentTime();
        tmpIFuseConn->inuseCnt++;
        
        g_InUseStatConn = tmpIFuseConn;
        *iFuseConn = tmpIFuseConn;
        
        pthread_mutex_unlock(&g_ConnectedConnLock);
        return 0;
    } else {
        // Decide whether creating a new connection or reuse one of existing connections
        targetIndex = -1;
        for(i=0;i<g_maxConnNum;i++) {
            if(g_InUseConn[i] == NULL) {
                targetIndex = i;
                break;
            }
        }
        
        if(targetIndex >= 0) {
            if (!g_FreeConn.empty()) {
                // reuse existing connection
                tmpIFuseConn = g_FreeConn.front();
                pthread_mutex_lock(&tmpIFuseConn->lock);
                tmpIFuseConn->actTime = iFuseLibGetCurrentTime();
                tmpIFuseConn->inuseCnt++;
                pthread_mutex_unlock(&tmpIFuseConn->lock);
                
                *iFuseConn = tmpIFuseConn;

                g_InUseConn[targetIndex] = tmpIFuseConn;
                g_FreeConn.remove(tmpIFuseConn);

                pthread_mutex_unlock(&g_ConnectedConnLock);
                return 0;
            } else {
                // create new
                status = _newConn(&tmpIFuseConn);
                if (status < 0) {
                    _freeConn(tmpIFuseConn);
                    pthread_mutex_unlock(&g_ConnectedConnLock);
                    return status;
                }
                
                tmpIFuseConn->actTime = iFuseLibGetCurrentTime();
                tmpIFuseConn->inuseCnt++;
                
                g_InUseConn[targetIndex] = tmpIFuseConn;
                *iFuseConn = tmpIFuseConn;
                
                pthread_mutex_unlock(&g_ConnectedConnLock);
                return 0;
            }
        } else {
            // reuse existing connection
            inUseCount = -1;
            tmpIFuseConn = NULL;
            for(i=0;i<g_maxConnNum;i++) {
                if(g_InUseConn[i] != NULL) {
                    if(inUseCount < 0) {
                        inUseCount = g_InUseConn[i]->inuseCnt;
                        tmpIFuseConn = g_InUseConn[i];
                    } else {
                        if(inUseCount > g_InUseConn[i]->inuseCnt) {
                            inUseCount = g_InUseConn[i]->inuseCnt;
                            tmpIFuseConn = g_InUseConn[i];
                        }
                    }
                }
            }
            
            assert(tmpIFuseConn != NULL);
            
            pthread_mutex_lock(&tmpIFuseConn->lock);
            tmpIFuseConn->actTime = iFuseLibGetCurrentTime();
            tmpIFuseConn->inuseCnt++;
            pthread_mutex_unlock(&tmpIFuseConn->lock);
            
            *iFuseConn = tmpIFuseConn;
            
            pthread_mutex_unlock(&g_ConnectedConnLock);
            return 0;
        }
    }
}

/*
 * Decrease reference count
 */
int iFuseConnUnuse(iFuseConn_t *iFuseConn) {
    int i;
    
    assert(iFuseConn != NULL);
    
    pthread_mutex_lock(&g_ConnectedConnLock);
    pthread_mutex_lock(&iFuseConn->lock);
    
    iFuseConn->actTime = iFuseLibGetCurrentTime();
    iFuseConn->inuseCnt--;
    
    assert(iFuseConn->inuseCnt >= 0);
    
    if(iFuseConn->inuseCnt == 0) {
        // move to free list
        if(g_InUseStatConn == iFuseConn) {
            g_InUseStatConn = NULL;
            
            assert(g_FreeStatConn == NULL);
            g_FreeStatConn = iFuseConn;
        } else {
            for(i=0;i<g_maxConnNum;i++) {
                if(g_InUseConn[i] == iFuseConn) {
                    g_InUseConn[i] = NULL;
                    break;
                }
            }
            
            g_FreeConn.push_back(iFuseConn);
        }
    }

    pthread_mutex_unlock(&iFuseConn->lock);
    pthread_mutex_unlock(&g_ConnectedConnLock);
    return 0;
}

/*
 * Reconnect
 */
int iFuseConnReconnect(iFuseConn_t *iFuseConn) {
    int status = 0;
    assert(iFuseConn != NULL);
    
    pthread_mutex_lock(&iFuseConn->lock);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFuseConnReconnect: disconnecting - %lu", iFuseConn->connId);
    _disconnect(iFuseConn);
    
    iFuseRodsClientLog(LOG_DEBUG, "iFuseConnReconnect: connecting - %lu", iFuseConn->connId);
    status = _connect(iFuseConn);
    
    pthread_mutex_unlock(&iFuseConn->lock);
    
    return status;
}

/*
 * Lock connection
 */
void iFuseConnLock(iFuseConn_t *iFuseConn) {
    assert(iFuseConn != NULL);
    
    pthread_mutex_lock(&iFuseConn->lock);

    rodsLog(LOG_DEBUG, "iFuseConnLock: connection locked - %lu", iFuseConn->connId);
}

/*
 * Unlock connection
 */
void iFuseConnUnlock(iFuseConn_t *iFuseConn) {
    assert(iFuseConn != NULL);
    
    pthread_mutex_unlock(&iFuseConn->lock);

    rodsLog(LOG_DEBUG, "iFuseConnUnlock: connection unlocked - %lu", iFuseConn->connId);
}
