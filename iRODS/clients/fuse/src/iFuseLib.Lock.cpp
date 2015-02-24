/*** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.hpp"
#include "iFuseLib.hpp"
#include "iFuseOper.hpp"
#include "hashtable.hpp"
#include "list.hpp"
#include "iFuseLib.Lock.hpp"
#undef USE_BOOST


#ifdef USE_BOOST
/*boost::mutex DescLock;*/
/* boost::mutex ConnLock;*/
boost::mutex* PathCacheLock = new boost::mutex();
/* boost::mutex FileCacheLock; */
boost::thread*            ConnManagerThr;
boost::mutex*             ConnManagerLock = new boost::mutex();
boost::mutex*             WaitForConnLock = new boost::mutex();
boost::condition_variable ConnManagerCond;
#else
/*pthread_mutex_t DescLock;*/
/*pthread_mutex_t ConnLock;*/
//pthread_mutex_t PathCacheLock;
/* pthread_mutex_t FileCacheLock; */
pthread_t ConnManagerThr;
pthread_mutex_t ConnManagerLock;
pthread_mutex_t WaitForConnLock;
pthread_cond_t ConnManagerCond;
#endif

#ifdef USE_BOOST
/*#define LOCK(Lock) ((Lock).lock())
#define UNLOCK(Lock) ((Lock).unlock())
#define INIT_STRUCT_LOCK(s) ((s).mutex = new boost::mutex) // JMC :: necessary since no ctor/dtor on struct
#define LOCK_STRUCT(s) LOCK(*((s).mutex))
#define UNLOCK_STRUCT(s) UNLOCK(*((s).mutex))
#define FREE_STRUCT_LOCK(s) \
    delete (s).mutex; \
    (s).mutex = 0;*/

void releaseFuseConnLock( iFuseConn_t *tmpIFuseConn ) {
    /* don't unlock. it will cause delete to fail */
    FREE_STRUCT_LOCK( *tmpIFuseConn );
}

void initConnReqWaitMutex( connReqWait_t *myConnReqWait ) {
    myConnReqWait->mutex = new boost::mutex;

}

void deleteConnReqWaitMutex( connReqWait_t *myConnReqWait ) {
    delete myConnReqWait->mutex;

}

void timeoutWait( boost::mutex** ConnManagerLock, boost::condition_variable *ConnManagerCond, int sleepTime ) {
    boost::system_time const tt = boost::get_system_time() + boost::posix_time::seconds( sleepTime );
    boost::unique_lock< boost::mutex > boost_lock( **ConnManagerLock );
    ConnManagerCond->timed_wait( boost_lock, tt );
}
void untimedWait( boost::mutex** ConnManagerLock, boost::condition_variable *ConnManagerCond ) {
    boost::unique_lock< boost::mutex > boost_lock( **ConnManagerLock );
    ConnManagerCond->wait( boost_lock );
}
void notifyWait( boost::mutex **ConnManagerLock, boost::condition_variable *ConnManagerCond ) {
    ConnManagerCond->notify_all( );
}
#else
/*#define UNLOCK(Lock) (pthread_mutex_unlock (&(Lock)))
#define LOCK(Lock) (pthread_mutex_lock (&(Lock)))
#define INIT_STRUCT_LOCK(s) (pthread_mutex_init (&((s).lock), NULL))
#define LOCK_STRUCT(s) LOCK((s).lock)
#define UNLOCK_STRUCT(s) UNLOCK((s).lock)
#define FREE_STRUCT_LOCK(s) \
    pthread_mutex_destroy (&((s).lock))*/
void releaseFuseConnLock( iFuseConn_t *tmpIFuseConn ) {
UNLOCK_STRUCT( *tmpIFuseConn );
FREE_STRUCT_LOCK( *tmpIFuseConn );
}

void initConnReqWaitMutex( connReqWait_t *myConnReqWait ) {
pthread_mutex_init( &myConnReqWait->mutex, NULL );
pthread_cond_init( &myConnReqWait->cond, NULL );

}

void deleteConnReqWaitMutex( connReqWait_t *myConnReqWait ) {
pthread_mutex_destroy( &myConnReqWait->mutex );

}

void timeoutWait( pthread_mutex_t *ConnManagerLock, pthread_cond_t *ConnManagerCond, int sleepTime ) {
struct timespec timeout;
bzero( &timeout, sizeof( timeout ) );
timeout.tv_sec = time( 0 ) + sleepTime;
pthread_mutex_lock( ConnManagerLock );
pthread_cond_timedwait( ConnManagerCond, ConnManagerLock, &timeout );
pthread_mutex_unlock( ConnManagerLock );
}
void untimedWait( pthread_mutex_t *ConnManagerLock, pthread_cond_t *ConnManagerCond ) {
pthread_mutex_lock( ConnManagerLock );
pthread_cond_wait( ConnManagerCond, ConnManagerLock );
pthread_mutex_unlock( ConnManagerLock );
}
void notifyWait( pthread_mutex_t *mutex, pthread_cond_t *cond ) {
pthread_mutex_lock( mutex );
pthread_cond_signal( cond );
pthread_mutex_unlock( mutex );
}
#endif
