/* For more information please refer to files in the COPYRIGHT directory ***/

/*#define FUSE_DEBUG 1*/

/*
 * Locking order:
 *
 * PathCacheLock (we allow getAndUseConnByPath as an exception, todo add a separate cache for this purpose
 * PathCache
 * DescLock
 * iFuseDesc
 * FileCache
 * iFuseConn struct
 * lock for concurrent queue in iFuseLib.Conn.c
 *
 *
 * Invariants:
 *
 */

#ifndef IFUSELIB_LOCK_HPP_
#define IFUSELIB_LOCK_HPP_
#include "irodsFs.hpp"
#include "iFuseLib.hpp"

#undef USE_BOOST

typedef struct ConcurrentList {
    List *list;
#ifdef USE_BOOST
    boost::mutex* mutex;
#else
    pthread_mutex_t lock;
#endif
} concurrentList_t;


#ifdef USE_BOOST

#include <boost/thread/thread_time.hpp>
extern boost::mutex* PathCacheLock;
extern boost::thread*            ConnManagerThr;
extern boost::mutex*             ConnManagerLock;
extern boost::mutex*             WaitForConnLock;
extern boost::condition_variable ConnManagerCond;
#else
#include <pthread.h>
extern pthread_mutex_t PathCacheLock;
extern pthread_t ConnManagerThr;
extern pthread_mutex_t ConnManagerLock;
extern pthread_mutex_t WaitForConnLock;
extern pthread_cond_t ConnManagerCond;
#endif

#ifdef USE_BOOST
#define LOCK(Lock) ((Lock)->lock())
#define UNLOCK(Lock) ((Lock)->unlock())
#define INIT_STRUCT_LOCK(s) INIT_LOCK((s).mutex) // JMC :: necessary since no ctor/dtor on struct
#define INIT_LOCK(Lock) ((Lock) = new boost::mutex) // JMC :: necessary since no ctor/dtor on struct
#define FREE_LOCK(Lock) \
	    delete (Lock); \
	    (Lock) = 0;

#define LOCK_STRUCT(s) LOCK(((s).mutex))
#define UNLOCK_STRUCT(s) UNLOCK(((s).mutex))
#define FREE_STRUCT_LOCK(s) \
	FREE_LOCK((s).mutex);

void releaseFuseConnLock( iFuseConn_t *tmpIFuseConn );

void initConnReqWaitMutex( connReqWait_t *myConnReqWait );
void deleteConnReqWaitMutex( connReqWait_t *myConnReqWait );
void timeoutWait( boost::mutex **ConnManagerLock, boost::condition_variable *ConnManagerCond, int sleepTime );
void untimedWait( boost::mutex **ConnManagerLock, boost::condition_variable *ConnManagerCond );
void notifyWait( boost::mutex **ConnManagerLock, boost::condition_variable *ConnManagerCond );
#else
#ifdef FUSE_DEBUG
#define UNLOCK(Lock) \
		if(FUSE_DEBUG)rodsLog(LOG_ERROR, "[ LOCK ]%s:%d %p", __FILE__, __LINE__, &(Lock)); \
	(pthread_mutex_unlock (&(Lock)))

#define LOCK(Lock) \
		if(FUSE_DEBUG)rodsLog(LOG_ERROR, "[UNLOCK] %s:%d %p", __FILE__, __LINE__, &(Lock)); \
	(pthread_mutex_lock (&(Lock)))

#else
#define UNLOCK(Lock) (pthread_mutex_unlock (&(Lock)))
#define LOCK(Lock) (pthread_mutex_lock (&(Lock)))
#endif
#define INIT_STRUCT_LOCK(s) INIT_LOCK((s).lock)
#define INIT_LOCK(s) (pthread_mutex_init (&(s), NULL))
#define LOCK_STRUCT(s) LOCK((s).lock)
#define UNLOCK_STRUCT(s) UNLOCK((s).lock)
#define FREE_STRUCT_LOCK(s) FREE_LOCK((s).lock)
#define FREE_LOCK(s) pthread_mutex_destroy (&(s))

void releaseFuseConnLock( iFuseConn_t *tmpIFuseConn );

void initConnReqWaitMutex( connReqWait_t *myConnReqWait );
void deleteConnReqWaitMutex( connReqWait_t *myConnReqWait );
void timeoutWait( pthread_mutex_t *ConnManagerLock, pthread_cond_t *ConnManagerCond, int sleepTime );
void untimedWait( pthread_mutex_t *ConnManagerLock, pthread_cond_t *ConnManagerCond );
void notifyWait( pthread_mutex_t *mutex, pthread_cond_t *cond );
#endif

#define FREE(s, t) _free##t(s);

#ifdef FUSE_DEBUG
#undef FREE
#define FREE(s, t) \
	rodsLog(LOG_ERROR, "[FREE "#t" %s:%d %p", __FILE__, __LINE__, s); \
	_free##t(s);
#endif

extern rodsEnv MyRodsEnv;
extern iFuseDesc_t IFuseDesc[MAX_IFUSE_DESC];
extern concurrentList_t *IFuseDescFreeList;

#define UNREF(s, t) \
	if(s != NULL) { \
		LOCK_STRUCT(*(s)); \
		(s)->status--; \
		if((s)->status == 0) { \
			UNLOCK_STRUCT(*(s)); \
			FREE_STRUCT_LOCK(*(s)); \
			FREE(s, t); \
		} else { \
			UNLOCK_STRUCT(*(s)); \
		} \
		(s) = NULL; \
	}

#define REF(v, s) \
	if(s != NULL) { \
		LOCK_STRUCT(*(s)); \
		(s)->status++; \
		UNLOCK_STRUCT(*(s)); \
		(v) = (s);\
	} else { \
		v = NULL; \
	}

#define REF_NO_LOCK(v, s) \
	if(s != NULL) { \
		(s)->status++; \
		(v) = (s);\
	} else { \
		v = NULL; \
	}

fileCache_t *newFileCache( int iFd, char *objPath, char *localPath, char *cacheFilePath, time_t cachedTime, int mode, rodsLong_t fileCache, cacheState_t state );
pathCache_t *newPathCache( char *inPath, fileCache_t *fileCache, struct stat *stbuf, time_t cachedTime );
iFuseConn_t *newIFuseConn( int *status );
iFuseDesc_t *newIFuseDesc( char *objPath, char *localPath, fileCache_t *fileCache, int *status );

int _freeFileCache( fileCache_t *fileCache );
int _freeIFuseConn( iFuseConn_t *conn );
/* precond: lock tmpPathCache */
int _freePathCache( pathCache_t *tmpPathCache );
int _freeIFuseDesc( iFuseDesc_t *desc );
int _freeIFuseConn( iFuseConn_t *tmpIFuseConn );

concurrentList_t *newConcurrentList();
void addToConcurrentList( concurrentList_t *l, void *v );
void removeFromConcurrentList2( concurrentList_t *l, void *v );
void removeFromConcurrentList( concurrentList_t *l, ListNode *v );
void* removeFirstElementOfConcurrentList( concurrentList_t *l );
void* removeLastElementOfConcurrentList( concurrentList_t *l );
int _listSize( concurrentList_t *l );
int listSize( concurrentList_t *l );

iFuseConn_t *getAndUseConnByPath( char *localPath, int *status );
int lookupPathNotExist( PathCacheTable *pctable, char *inPath );
int lookupPathExist( PathCacheTable *pctable, char *inPath, pathCache_t **paca );
pathCache_t *matchPathCache( PathCacheTable *pctable, const char *inPath );
int updatePathCacheStatFromFileCache( pathCache_t *tmpPathCache );
int clearPathFromCache( PathCacheTable *pctable, char *inPath );
int pathNotExist( PathCacheTable *pctable, char *inPath );
int pathExist( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache );
fileCache_t *addFileCache( int iFd, char *objPath, char *localPath, char *cachePath, int mode, rodsLong_t fileCache, cacheState_t state );
int addFileCacheForPath( pathCache_t *pathCache, fileCache_t *fileCache );
int pathReplace( PathCacheTable *pctable, char *inPath, fileCache_t *fileCache, struct stat *stbuf, pathCache_t **outPathCache );
int _getAndUseConnForPathCache( iFuseConn_t **iFuseConn, pathCache_t *paca );
int _getAndUseIFuseConn( iFuseConn_t **iFuseConn );
int getAndUseIFuseConn( iFuseConn_t **iFuseConn );

int iFuseFileCacheLseek( fileCache_t *fileCache, off_t offset );
int _iFuseFileCacheLseek( fileCache_t *fileCache, off_t offset );
int iFuseFileCacheFlush( fileCache_t *fileCache );
int _iFuseFileCacheFlush( fileCache_t *fileCache );
int ifuseFileCacheSwapOut( fileCache_t *fileCache );
/* close any open files inside the file cache */
int ifuseFileCacheClose( fileCache_t *fileCache );
int getFileCachePath( const char *inPath, char *cachePath );
int setAndMkFileCacheDir();
int ifuseFileCacheWrite( fileCache_t *fileCache, char *buf, size_t size, off_t offset );
int ifuseFileCacheRead( fileCache_t *fileCache, char *buf, size_t size, off_t offset );

void _ifuseDisconnect( iFuseConn_t *tmpIFuseConn );

int _ifuseRead( iFuseDesc_t *desc, char *buf, size_t size, off_t offset );
int
_ifuseWrite( iFuseDesc_t *desc, char *buf, size_t size, off_t offset );
int
_ifuseLseek( iFuseDesc_t *desc, off_t offset );

/* dummy function for reusing hashtable.c from RE */
char *cpStringExt( char *str, Region *r );
#endif

