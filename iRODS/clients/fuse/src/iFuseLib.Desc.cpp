/*** For more information please refer to files in the COPYRIGHT directory ***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "irodsFs.hpp"
#include "iFuseLib.hpp"
#include "iFuseOper.hpp"
#include "irods_hashtable.h"
#include "irods_list.h"

#undef USE_BOOST

/* Lock DescLock when this array is read/writen, or the inUseFlag var in an array element is read/written.
 * Locking DescLock prevent allocating/freeing new IFuseDesc.
 * After being added to the pathCache or newlyCreatedCache,
 * LOCK_STRUCT when an array element (except its inUseFlag) is read/written */
iFuseDesc_t IFuseDesc[MAX_IFUSE_DESC];
concurrentList_t *IFuseDescFreeList;

int
initIFuseDesc() {
#ifndef USE_BOOST
    //pthread_mutex_init( &PathCacheLock, NULL );
    pthread_mutex_init( &ConnManagerLock, NULL );
    pthread_cond_init( &ConnManagerCond, NULL );
    IFuseDescFreeList = newConcurrentList();
    int i;
    for ( i = 0; i < MAX_IFUSE_DESC; i++ ) {
        INIT_STRUCT_LOCK( IFuseDesc[i] );
        IFuseDesc[i].index = i;
        if ( i >= 3 ) {
            addToConcurrentList( IFuseDescFreeList, &IFuseDesc[i] );
        }
    }
#endif
    // JMC - overwrites objects construction? -  memset (IFuseDesc, 0, sizeof (iFuseDesc_t) * MAX_IFUSE_DESC);
    return 0;
}

int _lockDesc( iFuseDesc_t *desc ) {

    int status = 0;

#ifdef USE_BOOST
    try {
        desc->mutex->lock();
    }
    catch ( const boost::thread_resource_error& ) {
        status = -1;
    }
#else
    status = pthread_mutex_lock( &desc->lock );
#endif
    return status;
}

int
lockDesc( int descInx ) {
    int status;

    if ( descInx < 3 || descInx >= MAX_IFUSE_DESC ) {
        rodsLog( LOG_ERROR,
                 "lockDesc: descInx %d out of range", descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }
    status = _lockDesc( &IFuseDesc[descInx] );
    return status;
}

int _unlockDesc( iFuseDesc_t *desc ) {
    int status;
#ifdef USE_BOOST
    try {
        desc->mutex->unlock();
    }
    catch ( const boost::thread_resource_error& ) {
        status = -1;
    }
#else
    status = pthread_mutex_unlock( &desc->lock );
#endif
    return status;
}

int unlockDesc( int descInx ) {
    int status;

    if ( descInx < 3 || descInx >= MAX_IFUSE_DESC ) {
        rodsLog( LOG_ERROR,
                 "unlockDesc: descInx %d out of range", descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }
    status = _unlockDesc( &IFuseDesc[descInx] );
    return status;
}

int
checkFuseDesc( int descInx ) {
    if ( descInx < 3 || descInx >= MAX_IFUSE_DESC ) {
        rodsLog( LOG_ERROR,
                 "checkFuseDesc: descInx %d out of range", descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    return 0;
}

/* close a iFuse file */
int
ifuseClose( iFuseDesc_t *desc ) {
    int status;

    LOCK_STRUCT( *desc );

    status = _ifuseFlush( desc );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseClose: flush of %s error",
                      desc->localPath );

    }
    status = ifuseFileCacheClose( desc->fileCache );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseClose: close of %s error",
                      desc->localPath );

    }
    status = _freeIFuseDesc( desc );

    UNLOCK_STRUCT( *desc );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseClose: free desc struct error",
                      desc->localPath );

    }

    return status;
}
int
ifuseFlush( int descInx ) {
    int status;
    LOCK_STRUCT( IFuseDesc[descInx] );
    status = _ifuseFlush( &IFuseDesc[descInx] );
    UNLOCK_STRUCT( IFuseDesc[descInx] );

    return status;
}


/* precond: lock desc */
int
_ifuseFlush( iFuseDesc_t *desc ) {
    int status;
    status = iFuseFileCacheFlush( desc->fileCache );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseFlush: flush of %s error",
                      desc->localPath );

    }

    return 0;

}



/* precond: lock desc */
int
_ifuseWrite( iFuseDesc_t *desc, char *buf, size_t size, off_t offset ) {
    int status = ifuseFileCacheWrite( desc->fileCache, buf, size, offset );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseWrite: write of %s error",
                      desc->localPath );

    }
    return status;
}


/* precond: lock desc */
int _ifuseRead( iFuseDesc_t *desc, char *buf, size_t size, off_t offset ) {
    int status = ifuseFileCacheRead( desc->fileCache, buf, size, offset );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "ifuseRead: read of %s error",
                      desc->localPath );

    }
    return status;
}


/* precond: lock desc */
int
_ifuseLseek( iFuseDesc_t *desc, off_t offset ) {
    int status;


    if ( desc->offset != offset ) {
        status = iFuseFileCacheLseek( desc->fileCache, offset );

        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status, "ifuseLseek: lseek of %s error",
                          desc->localPath );
            return status;
        }
        else {
            desc->offset = offset;
        }

    }
    return 0;
}
