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

fileCache_t *newFileCache( int iFd, char *objPath, char *localPath, char *cacheFilePath, time_t cachedTime, int mode, rodsLong_t fileSize, cacheState_t state ) {
    fileCache_t *fileCache = ( fileCache_t * ) malloc( sizeof( fileCache_t ) );
    if ( fileCache == NULL ) {
        return NULL;
    }
    fileCache->fileCachePath = cacheFilePath == NULL ? NULL : strdup( cacheFilePath );
    fileCache->localPath = strdup( localPath );
    fileCache->objPath = strdup( objPath );
    fileCache->cachedTime = cachedTime;
    fileCache->mode = mode;
    fileCache->fileSize = fileSize;
    fileCache->iFd = iFd;
    fileCache->state = state;
    fileCache->status = 0;
    fileCache->offset = 0;
    INIT_STRUCT_LOCK( *fileCache );
    return fileCache;
}

pathCache_t *newPathCache( char *inPath, fileCache_t *fileCache, struct stat *stbuf, time_t cachedTime ) {
    pathCache_t *tmpPathCache;

    tmpPathCache = ( pathCache_t * ) malloc( sizeof( pathCache_t ) );
    if ( tmpPathCache == NULL ) {
        return NULL;
    }

    tmpPathCache->localPath = strdup( inPath );
    tmpPathCache->cachedTime = cachedTime;
    tmpPathCache->expired = 0;
    REF( tmpPathCache->fileCache, fileCache );
    tmpPathCache->iFuseConn = NULL;
    INIT_STRUCT_LOCK( *tmpPathCache );
    if ( stbuf != NULL ) {
        tmpPathCache->stbuf = *stbuf;
    }
    else {
        bzero( &( tmpPathCache->stbuf ), sizeof( struct stat ) );
    }
    return tmpPathCache;
}

iFuseConn_t *newIFuseConn( int *status ) {
    iFuseConn_t *tmpIFuseConn = ( iFuseConn_t * ) malloc( sizeof( iFuseConn_t ) );
    if ( tmpIFuseConn == NULL ) {
        *status = SYS_MALLOC_ERR;
        return NULL;
    }
    bzero( tmpIFuseConn, sizeof( iFuseConn_t ) );

    INIT_STRUCT_LOCK( *tmpIFuseConn );

    *status = ifuseConnect( tmpIFuseConn, &MyRodsEnv );
    /*rodsLog(LOG_ERROR, "[ NEW IFUSE CONN] %s:%d %p", __FILE__, __LINE__, tmpIFuseConn);*/
    return tmpIFuseConn;

}
/* precond: fileCache locked or single thread use */
iFuseDesc_t *newIFuseDesc( char *objPath, char *localPath, fileCache_t *fileCache, int *status ) {
    iFuseDesc_t *desc;
    LOCK_STRUCT( *IFuseDescFreeList );
    /*
    printf("******************************************************\n");
    printf("num of desc slots free = %d\n", _listSize(IFuseDescFreeList));
    printf("******************************************************\n"); */
    if ( _listSize( IFuseDescFreeList ) != 0 ) {
        ListNode *node = IFuseDescFreeList->list->head;
        desc = ( iFuseDesc_t * ) node->value;
        listRemoveNoRegion( IFuseDescFreeList->list, node );
        UNLOCK_STRUCT( *IFuseDescFreeList );
        REF_NO_LOCK( desc->fileCache, fileCache );
        desc->objPath = strdup( objPath );
        desc->localPath = strdup( localPath );
        desc->offset = 0;
        INIT_STRUCT_LOCK( *desc );
        *status = 0;
        return desc;
    }
    else {
        UNLOCK_STRUCT( *IFuseDescFreeList );
        rodsLog( LOG_ERROR,
                 "allocIFuseDesc: Out of iFuseDesc" );
        *status = SYS_OUT_OF_FILE_DESC;

        return NULL;
    }
}

int _freeFileCache( fileCache_t *fileCache ) {
    free( fileCache->fileCachePath );
    free( fileCache->localPath );
    free( fileCache->objPath );
    FREE_STRUCT_LOCK( *fileCache );
    free( fileCache );
    return 0;
}

/* precond: lock tmpPathCache */
int _freePathCache( pathCache_t *tmpPathCache ) {
    free( tmpPathCache->localPath );
    if ( tmpPathCache->fileCache != NULL ) {
        UNREF( tmpPathCache->fileCache, FileCache );
    }
    FREE_STRUCT_LOCK( *tmpPathCache );
    free( tmpPathCache );
    return 0;
}

/* precond: lock desc */
int _freeIFuseDesc( iFuseDesc_t *desc ) {
    int i;

    if ( desc->index < 3 ) {
        rodsLog( LOG_ERROR,
                 "freeIFuseDesc: descInx %d out of range", desc->index );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    for ( i = 0; i < MAX_BUF_CACHE; i++ ) {
        if ( desc->bufCache[i].buf != NULL ) {
            free( desc->bufCache[i].buf );
        }
    }
    free( desc->objPath );

    free( desc->localPath );

    UNREF( desc->fileCache, FileCache );

    addToConcurrentList( IFuseDescFreeList, desc );
    /*
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printf("num of desc slots free = %d\n", listSize(IFuseDescFreeList));
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");*/

    return 0;

}
int _freeIFuseConn( iFuseConn_t *tmpIFuseConn ) {
    _ifuseDisconnect( tmpIFuseConn );
    FREE_STRUCT_LOCK( *tmpIFuseConn );
    free( tmpIFuseConn );
    return 0;

}

concurrentList_t *newConcurrentList() {
    List *l = newListNoRegion();
    concurrentList_t *cl = ( concurrentList_t * ) malloc( sizeof( concurrentList_t ) );
    cl->list = l;
    INIT_STRUCT_LOCK( *cl );
    return cl;
}
/*#define FUSE_DEBUG 0*/
void addToConcurrentList( concurrentList_t *l, void *v ) {
    LOCK_STRUCT( *l );
    listAppendNoRegion( l->list, v );
    UNLOCK_STRUCT( *l );
}
void removeFromConcurrentList2( concurrentList_t *l, void *v ) {
    LOCK_STRUCT( *l );
    listRemoveNoRegion2( l->list, v );
    UNLOCK_STRUCT( *l );

}
void removeFromConcurrentList( concurrentList_t *l, ListNode *v ) {
    LOCK_STRUCT( *l );
    listRemoveNoRegion( l->list, v );
    UNLOCK_STRUCT( *l );
}
void* removeFirstElementOfConcurrentList( concurrentList_t *l ) {
    LOCK_STRUCT( *l );
    void* tmp;
    if ( l->list->head == NULL ) {
        tmp = NULL;
    }
    else {
        tmp = l->list->head->value;
        listRemoveNoRegion( l->list, l->list->head );
    }
    UNLOCK_STRUCT( *l );
    return tmp;
}
void *removeLastElementOfConcurrentList( concurrentList_t *l ) {
    LOCK_STRUCT( *l );
    void* tmp;
    if ( l->list->head == NULL ) {
        tmp = NULL;
    }
    else {
        tmp = l->list->tail->value;
        listRemoveNoRegion( l->list, l->list->tail );
    }
    UNLOCK_STRUCT( *l );
    return tmp;

}
int _listSize( concurrentList_t *l ) {
    return l->list->size;
}

int listSize( concurrentList_t *l ) {
    LOCK_STRUCT( *l );
    int s = _listSize( l );
    UNLOCK_STRUCT( *l );
    return s;
}

void deleteConcurrentList( concurrentList_t *l ) {
    deleteListNoRegion( l->list );
    free( l );
}

/*#define FUSE_DEBUG 1*/

/* dummy function */
char *cpStringExt( const char*, Region* ) {
    return NULL;
}
