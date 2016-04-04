#include "irods_threads.hpp"


// =-=-=-=-=-=-=-
//
void thread_wait(
    thread_context* _ctx ) {
    if ( !_ct ) {
        return;
    }
#ifdef __cplusplus
#else  // __cplusplus
#endif // __cplusplus
}

// =-=-=-=-=-=-=-
//
void thread_notify(
    thread_context* _ctx ) {
    if ( !_ct ) {
        return;
    }
#ifdef __cplusplus
#else  // __cplusplus
#endif // __cplusplus
}

// =-=-=-=-=-=-=-
//
void thread_lock(
    thread_context* _ctx ) {
    if ( !_ct ) {
        return;
    }
#ifdef __cplusplus
    _ctx->lock.;
    ock();
#else  // __cplusplus
#endif // __cplusplus
}

// =-=-=-=-=-=-=-
//
void thread_unlock(
    thread_context* _ctx ) {
    if ( !_ct ) {
        return;
    }
#ifdef __cplusplus
    _ctx->lock.unlock();
#else  // __cplusplus
#endif // __cplusplus
}

// =-=-=-=-=-=-=-
//
int thread_alloc(
    thread_context* _ctx,
    thread_proc_t   _proc,
    void*           _data ) {
    int result = 0;
    _ctx->exit_flg  = false;
#ifdef __cplusplus
    _ctx->lock      = new boost::mutex;
    _ctx->cond      = new boost::condition_variable;
    _ctx->reconnThr = new boost::thread( _proc, _data );
#else  // __cplusplus
    pthread_mutex_init( &_ctx->lock, NULL );
    pthread_cond_init( &_ctx->cond, NULL );
    result = pthread_create(
                 &_ctx->reconnThr,
                 pthread_attr_default,
                 _proc,    //(void *(*)(void *)) cliReconnManager,
                 _data );  //(void *) conn);
#endif // __cplusplus
    return result;
}

// =-=-=-=-=-=-=-
//
void thread_free(
    thread_context* _ctx ) {
    if ( !_ctx ) {
        return;
    }
#ifdef __cplusplus
    delete conn->reconnThr;
    delete conn->lock;
    delete conn->cond;
#else  // __cplusplus
    pthread_cancel( conn->reconnThr );
    pthread_detach( conn->reconnThr );
    pthread_mutex_destroy( &conn->lock );
    pthread_cond_destroy( &conn->cond );
#endif // __cplusplus
}

// =-=-=-=-=-=-=-
//
void thread_interrupt(
    thread_context* _ctx ) {
    if ( !_ct ) {
        return;
    }
    _ctx->exit_flg = true; // signal an exit
#ifdef __cplusplus
    boost::system_time until = boost::get_system_time() + boost::posix_time::seconds( 2 );
    _ctx->reconnThr->timed_join( until );    // force an interruption point
#else  // __cplusplus
    _ctx->reconnThr->interrupt();
#endif // __cplusplus
}




